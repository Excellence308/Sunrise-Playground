#include "activity_sensor_sense_observer.h"
#include "activity_sensor_sense_schema_validator.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string_view>

#include "../../../../core/logging/log.h"
#include "../../../../state/build_data/runtime.h"

namespace sunrise::server::bap::encrypted::activity_message {
namespace {

/** Exact roster keys already established from the installed Shadowkeep package set. */
constexpr std::string_view kHallDestination = "trophy_hall_freeroam";
constexpr std::uint32_t kPrimaryGroup = 0x4786C0E0;
constexpr std::uint32_t kAmbientGroup = 0xF18B720F;
constexpr std::size_t kPrimarySlotCount = 21;
constexpr std::size_t kAmbientSlotCount = 146;
constexpr std::size_t kGroupKeyWidth = 32;
/** Proven by the refined live run: type begins immediately after the 32-bit key. */
constexpr std::size_t kSlotTypeWidth = 7;
constexpr std::size_t kSlotIndexWidth = 16;
constexpr std::size_t kObjectIdentityWidth = kGroupKeyWidth + kSlotTypeWidth + kSlotIndexWidth;
constexpr std::uint32_t kSlotTypeBias = 1;
constexpr std::uint32_t kSlotIndexBias = 32768;
/** One continuation bit precedes every object key. */
constexpr std::size_t kObjectContinuationWidth = 1;
/** A group header is its key, a discarded 32-bit filler, then the first continuation bit. */
constexpr std::size_t kGroupHeaderToFirstObjectBits = 65;
/** Keeps this temporary parser bounded even if another activity emits unusually large updates. */
constexpr unsigned kMessageBudget = 16;
constexpr std::size_t kMaximumPayloadBytes = 16 * 1024;
constexpr std::size_t kMaximumKnownKeysPerMessage = 256;

std::atomic_uint g_observedMessages{};

using RosterGroup = state::build_data::scenarios::RosterGroup;

struct KeyMatch final {
    std::uint32_t key{};
    std::size_t bit{};
    std::uint8_t type{};
    std::uint16_t index{};
    bool object{};
};

struct ParseSummary final {
    std::size_t knownKeys{};
    std::size_t storedKeys{};
    std::size_t objects{};
    std::size_t framedObjects{};
    std::size_t observedVariants{};
    std::size_t groupHeaders{};
    std::size_t schemaExact{};
    std::size_t schemaPartial{};
    std::size_t schemaMismatch{};
    std::size_t schemaUnsupported{};
};

/** Copies the two exact Hall groups from Sunrise's immutable extracted scenario catalog. */
[[nodiscard]] bool load_hall_groups(RosterGroup& primary, RosterGroup& ambient) noexcept {
    primary = {};
    ambient = {};
    state::build_data::scenarios::Definition layout{};
    if (!state::build_data::find_scenario_layout(kHallDestination, layout)) {
        return false;
    }
    bool foundPrimary = false;
    bool foundAmbient = false;
    for (std::size_t row = 0; row < layout.rosterGroupCount; ++row) {
        RosterGroup candidate{};
        if (!state::build_data::find_roster_group(layout.rosterGroups[row], candidate)) {
            return false;
        }
        if (candidate.registryKey == kPrimaryGroup && candidate.slotCount == kPrimarySlotCount) {
            primary = candidate;
            foundPrimary = true;
        } else if (candidate.registryKey == kAmbientGroup
                   && candidate.slotCount == kAmbientSlotCount) {
            ambient = candidate;
            foundAmbient = true;
        }
    }
    return foundPrimary && foundAmbient;
}

/** @return One MSB-first bit from checked borrowed storage. */
[[nodiscard]] bool
bit_at(std::span<const std::byte> payload, std::size_t bit, std::uint32_t& value) noexcept {
    if (payload.size() > (std::numeric_limits<std::size_t>::max)() / 8
        || bit >= payload.size() * 8) {
        value = 0;
        return false;
    }
    const std::size_t byte = bit / 8;
    const std::size_t shift = 7 - (bit % 8);
    value = (std::to_integer<std::uint32_t>(payload[byte]) >> shift) & 1U;
    return true;
}

/** Reads one checked MSB-first field without retaining its source bytes. */
[[nodiscard]] bool read_at(std::span<const std::byte> payload,
                           std::size_t bit,
                           std::size_t width,
                           std::uint64_t& value) noexcept {
    value = 0;
    if (width > 64 || payload.size() > (std::numeric_limits<std::size_t>::max)() / 8) {
        return false;
    }
    const std::size_t bits = payload.size() * 8;
    if (bit > bits || width > bits - bit) {
        return false;
    }
    for (std::size_t index = 0; index < width; ++index) {
        std::uint32_t next = 0;
        if (!bit_at(payload, bit + index, next)) {
            return false;
        }
        value = (value << 1U) | next;
    }
    return true;
}

/** @return The immutable roster named by one known Hall key, or null for another key. */
[[nodiscard]] const RosterGroup*
roster_for(std::uint32_t key, const RosterGroup& primary, const RosterGroup& ambient) noexcept {
    if (key == primary.registryKey) {
        return &primary;
    }
    if (key == ambient.registryKey) {
        return &ambient;
    }
    return nullptr;
}

/** @return True when the fields immediately after a key name one exact installed roster slot. */
[[nodiscard]] bool decode_object(std::span<const std::byte> payload,
                                 KeyMatch& match,
                                 const RosterGroup& roster) noexcept {
    std::uint64_t typeWire = 0;
    std::uint64_t indexWire = 0;
    const std::size_t typeBit = match.bit + kGroupKeyWidth;
    const std::size_t indexBit = typeBit + kSlotTypeWidth;
    if (!read_at(payload, typeBit, kSlotTypeWidth, typeWire)
        || !read_at(payload, indexBit, kSlotIndexWidth, indexWire) || typeWire < kSlotTypeBias
        || indexWire < kSlotIndexBias) {
        return false;
    }
    const std::uint64_t decodedType = typeWire - kSlotTypeBias;
    const std::uint64_t decodedIndex = indexWire - kSlotIndexBias;
    if (decodedType > (std::numeric_limits<std::uint8_t>::max)() || decodedIndex >= roster.slotCount
        || roster.slotTypes[static_cast<std::size_t>(decodedIndex)] != decodedType) {
        return false;
    }
    match.type = static_cast<std::uint8_t>(decodedType);
    match.index = static_cast<std::uint16_t>(decodedIndex);
    match.object = true;
    return true;
}

/** @return True only for a body width observed for this exact sense slot type. */
[[nodiscard]] bool observed_variant(std::uint8_t type, std::size_t bodyBits) noexcept {
    switch (type) {
    case 1:
        return bodyBits == 56 || bodyBits == 85 || bodyBits == 92;
    case 4:
        return bodyBits == 165;
    case 23:
        return bodyBits == 167;
    default:
        return false;
    }
}

[[nodiscard]] const char* schema_name(bool framed, SenseSchemaValidation validation) noexcept {
    if (!framed) {
        return "unframed";
    }
    switch (validation) {
    case SenseSchemaValidation::unsupported:
        return "unsupported";
    case SenseSchemaValidation::exact_record_prefix_map:
        return "exact_prefix_map";
    case SenseSchemaValidation::exact_record_trailer_map:
        return "exact_trailer_map";
    case SenseSchemaValidation::exact_map_ambiguous:
        return "exact_ambiguous_map";
    case SenseSchemaValidation::partial_dynamic:
        return "partial_kind22";
    case SenseSchemaValidation::mismatch:
        return "mismatch";
    }
    return "mismatch";
}

/** Reports one exact object record without reporting any of its sense-body bits. */
void report_record(unsigned ordinal,
                   const KeyMatch& match,
                   long long bodyBits,
                   std::size_t remainingBits,
                   bool knownVariant,
                   SenseSchemaValidation validation) noexcept {
    std::array<char, core::log::kLineCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=activity stage=sense_parse result=record ordinal=%u "
                                      "group=0x%08X key_bit=%zu type=%u index=%u body_bits=%lld "
                                      "remaining_bits=%zu framing=%s variant=%s schema=%s",
                                      ordinal,
                                      match.key,
                                      match.bit,
                                      static_cast<unsigned>(match.type),
                                      static_cast<unsigned>(match.index),
                                      bodyBits,
                                      remainingBits,
                                      bodyBits >= 0 ? "next_object" : "unframed_tail",
                                      knownVariant ? "observed" : "unresolved",
                                      schema_name(bodyBits >= 0, validation));
    if (written > 0) {
        core::log::write(core::log::Channel::server,
                         core::log::Level::info,
                         {line.data(), static_cast<std::size_t>(written)});
    }
}

/** Reports one key/filler/continuation group header proven by the following exact object. */
void report_group_header(unsigned ordinal, const KeyMatch& match) noexcept {
    std::array<char, core::log::kLineCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=activity stage=sense_parse result=group_header "
                                      "ordinal=%u group=0x%08X key_bit=%zu span_bits=%zu",
                                      ordinal,
                                      match.key,
                                      match.bit,
                                      kGroupHeaderToFirstObjectBits);
    if (written > 0) {
        core::log::write(core::log::Channel::server,
                         core::log::Level::info,
                         {line.data(), static_cast<std::size_t>(written)});
    }
}

/** Reports a bounded parse summary. */
void report_summary(unsigned ordinal, std::size_t bytes, const ParseSummary& summary) noexcept {
    std::array<char, core::log::kLineCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=activity stage=sense_parse result=summary ordinal=%u "
                                      "bytes=%zu keys=%zu stored=%zu objects=%zu framed=%zu "
                                      "observed_variants=%zu group_headers=%zu schema_exact=%zu "
                                      "schema_partial=%zu schema_mismatch=%zu schema_unsupported=%zu",
                                      ordinal,
                                      bytes,
                                      summary.knownKeys,
                                      summary.storedKeys,
                                      summary.objects,
                                      summary.framedObjects,
                                      summary.observedVariants,
                                      summary.groupHeaders,
                                      summary.schemaExact,
                                      summary.schemaPartial,
                                      summary.schemaMismatch,
                                      summary.schemaUnsupported);
    if (written > 0) {
        core::log::write(core::log::Channel::server,
                         core::log::Level::info,
                         {line.data(), static_cast<std::size_t>(written)});
    }
}

/** Reports a payload-safe reason why one bounded parse did not run. */
void report_skip(unsigned ordinal, std::size_t bytes, const char* reason) noexcept {
    std::array<char, core::log::kLineCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=activity stage=sense_parse result=skip ordinal=%u "
                                      "bytes=%zu reason=%s",
                                      ordinal,
                                      bytes,
                                      reason);
    if (written > 0) {
        core::log::write(core::log::Channel::server,
                         core::log::Level::info,
                         {line.data(), static_cast<std::size_t>(written)});
    }
}

} // namespace

/** Frames exact Hall sense records while retaining and reporting no sense-body data. */
void observe_sensor_sense_structure(std::span<const std::byte> payload) noexcept {
    const unsigned ordinal = g_observedMessages.fetch_add(1, std::memory_order_relaxed) + 1;
    if (ordinal > kMessageBudget) {
        return;
    }
    if (payload.empty() || payload.size() > kMaximumPayloadBytes
        || payload.size() > (std::numeric_limits<std::size_t>::max)() / 8) {
        report_skip(ordinal, payload.size(), "size");
        return;
    }

    RosterGroup primary{};
    RosterGroup ambient{};
    if (!load_hall_groups(primary, ambient)) {
        report_skip(ordinal, payload.size(), "roster");
        return;
    }

    ParseSummary summary{};
    std::array<KeyMatch, kMaximumKnownKeysPerMessage> matches{};
    std::uint32_t window = 0;
    const std::size_t payloadBits = payload.size() * 8;
    for (std::size_t bit = 0; bit < payloadBits; ++bit) {
        std::uint32_t next = 0;
        if (!bit_at(payload, bit, next)) {
            break;
        }
        window = (window << 1U) | next;
        if (bit + 1 < kGroupKeyWidth || (window != kPrimaryGroup && window != kAmbientGroup)) {
            continue;
        }
        ++summary.knownKeys;
        if (summary.storedKeys >= matches.size()) {
            continue;
        }
        KeyMatch& match = matches[summary.storedKeys++];
        match.key = window;
        match.bit = bit + 1 - kGroupKeyWidth;
    }

    for (std::size_t row = 0; row < summary.storedKeys; ++row) {
        KeyMatch& match = matches[row];
        const RosterGroup* roster = roster_for(match.key, primary, ambient);
        if (roster != nullptr) {
            (void)decode_object(payload, match, *roster);
        }
    }

    for (std::size_t row = 0; row < summary.storedKeys; ++row) {
        const KeyMatch& match = matches[row];
        if (!match.object) {
            const bool header =
                row + 1 < summary.storedKeys && matches[row + 1].object
                && matches[row + 1].key == match.key && matches[row + 1].bit > match.bit
                && matches[row + 1].bit - match.bit == kGroupHeaderToFirstObjectBits;
            if (header) {
                ++summary.groupHeaders;
                report_group_header(ordinal, match);
            }
            continue;
        }

        ++summary.objects;
        const std::size_t bodyBit = match.bit + kObjectIdentityWidth;
        const std::size_t remainingBits = bodyBit <= payloadBits ? payloadBits - bodyBit : 0;
        long long bodyBits = -1;
        bool knownVariant = false;
        SenseSchemaValidation validation = SenseSchemaValidation::unsupported;
        const bool hasNextObject = row + 1 < summary.storedKeys && matches[row + 1].object
                                   && matches[row + 1].key == match.key
                                   && matches[row + 1].bit > bodyBit;
        if (hasNextObject) {
            const std::size_t gap = matches[row + 1].bit - bodyBit;
            if (gap >= kObjectContinuationWidth) {
                const std::size_t framed = gap - kObjectContinuationWidth;
                bodyBits = static_cast<long long>(framed);
                knownVariant = observed_variant(match.type, framed);
                validation = validate_sensor_sense_body(payload, bodyBit, framed, match.type);
                ++summary.framedObjects;
                if (knownVariant) {
                    ++summary.observedVariants;
                }
                switch (validation) {
                case SenseSchemaValidation::exact_record_prefix_map:
                case SenseSchemaValidation::exact_record_trailer_map:
                case SenseSchemaValidation::exact_map_ambiguous:
                    ++summary.schemaExact;
                    break;
                case SenseSchemaValidation::partial_dynamic:
                    ++summary.schemaPartial;
                    break;
                case SenseSchemaValidation::mismatch:
                    ++summary.schemaMismatch;
                    break;
                case SenseSchemaValidation::unsupported:
                    ++summary.schemaUnsupported;
                    break;
                }
            }
        }
        report_record(ordinal, match, bodyBits, remainingBits, knownVariant, validation);
    }

    report_summary(ordinal, payload.size(), summary);
}

} // namespace sunrise::server::bap::encrypted::activity_message
