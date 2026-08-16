#include "activity_sensor_sense_schema_validator.h"

#include <array>
#include <limits>

namespace sunrise::server::bap::encrypted::activity_message {
namespace {

/** Checked MSB-first cursor over one borrowed record body. */
class Cursor final {
  public:
    Cursor(std::span<const std::byte> payload, std::size_t startBit, std::size_t bodyBits) noexcept
        : payload_(payload), position_(startBit) {
        if (payload.size() > (std::numeric_limits<std::size_t>::max)() / 8) {
            return;
        }
        const std::size_t payloadBits = payload.size() * 8;
        if (startBit > payloadBits || bodyBits > payloadBits - startBit) {
            return;
        }
        end_ = startBit + bodyBits;
        valid_ = true;
    }

    [[nodiscard]] bool skip(std::size_t width) noexcept {
        if (!valid_ || position_ > end_ || width > end_ - position_) {
            return false;
        }
        position_ += width;
        return true;
    }

    [[nodiscard]] bool presence(bool& present) noexcept {
        if (!valid_ || position_ >= end_) {
            present = false;
            return false;
        }
        const std::size_t byte = position_ / 8;
        const std::size_t shift = 7 - (position_ % 8);
        present = ((std::to_integer<unsigned>(payload_[byte]) >> shift) & 1U) != 0;
        ++position_;
        return true;
    }

    [[nodiscard]] bool at_end() const noexcept {
        return valid_ && position_ == end_;
    }

    [[nodiscard]] std::size_t remaining() const noexcept {
        return valid_ && position_ <= end_ ? end_ - position_ : 0;
    }

  private:
    std::span<const std::byte> payload_{};
    std::size_t position_{};
    std::size_t end_{};
    bool valid_{};
};

[[nodiscard]] SenseSchemaValidation layout_result(bool prefix, bool trailer) noexcept {
    if (prefix && trailer) {
        return SenseSchemaValidation::exact_map_ambiguous;
    }
    if (prefix) {
        return SenseSchemaValidation::exact_record_prefix_map;
    }
    if (trailer) {
        return SenseSchemaValidation::exact_record_trailer_map;
    }
    return SenseSchemaValidation::mismatch;
}

/** Walks schema 0x80807ECC with its eight optional markers grouped ahead of field values. */
[[nodiscard]] bool parse_type_1_map(Cursor cursor, bool recordPrefix) noexcept {
    constexpr std::array<std::size_t, 6> kOptionalScalarWidths{31, 31, 31, 6, 7, 31};
    std::array<bool, 8> present{};

    if (recordPrefix && !cursor.skip(1)) {
        return false;
    }
    for (bool& fieldPresent : present) {
        if (!cursor.presence(fieldPresent)) {
            return false;
        }
    }
    for (std::size_t field = 0; field < kOptionalScalarWidths.size(); ++field) {
        if (present[field] && !cursor.skip(kOptionalScalarWidths[field])) {
            return false;
        }
    }
    if (!cursor.skip(2) || !cursor.skip(3) || !cursor.skip(1) || !cursor.skip(1)
        || !cursor.skip(1)) {
        return false;
    }
    if (present[6] && (!cursor.skip(4) || !cursor.skip(32))) {
        return false;
    }
    if (present[7]) {
        bool scalarPresent = false;
        if (!cursor.presence(scalarPresent) || (scalarPresent && !cursor.skip(7))) {
            return false;
        }
    }
    return (recordPrefix || cursor.skip(1)) && cursor.at_end();
}

/** Tests both bounded placements of the common record bit around the type-1 presence map. */
[[nodiscard]] SenseSchemaValidation validate_type_1(const Cursor& cursor) noexcept {
    return layout_result(parse_type_1_map(cursor, true), parse_type_1_map(cursor, false));
}

/** Walks schema 0x80804F47 inline, inserting one opaque bit at the selected field boundary. */
[[nodiscard]] bool parse_type_23_inline_gap(Cursor cursor, std::size_t gap) noexcept {
    constexpr std::size_t kFieldCount = 6;
    for (std::size_t field = 0; field < kFieldCount; ++field) {
        if (gap == field && !cursor.skip(1)) {
            return false;
        }
        bool present = false;
        if (!cursor.presence(present) || (present && !cursor.skip(32))) {
            return false;
        }
    }
    if (gap == kFieldCount && !cursor.skip(1)) {
        return false;
    }
    return cursor.at_end();
}

/** Reports only which of the seven inline field boundaries close at the exact body end. */
[[nodiscard]] SenseSchemaResult validate_type_23(const Cursor& cursor) noexcept {
    SenseSchemaResult result{SenseSchemaValidation::mismatch, 0};
    for (std::size_t gap = 0; gap <= 6; ++gap) {
        if (parse_type_23_inline_gap(cursor, gap)) {
            result.type23GapMask |= static_cast<std::uint8_t>(1U << gap);
        }
    }
    if (result.type23GapMask == 0) {
        return result;
    }
    const std::uint8_t withoutLowest = static_cast<std::uint8_t>(
        result.type23GapMask & static_cast<std::uint8_t>(result.type23GapMask - 1U));
    result.validation = withoutLowest == 0 ? SenseSchemaValidation::exact_inline_gap
                                          : SenseSchemaValidation::exact_inline_gap_ambiguous;
    return result;
}

/** Schema 0x8080992E remains opaque after its 100 fixed top-level bits. */
[[nodiscard]] SenseSchemaValidation validate_type_4(Cursor& cursor) noexcept {
    // The remaining 65 bits combine one record bit with unresolved kind-0x22 state. This test
    // intentionally makes no claim about their order or values.
    return cursor.skip(100) && cursor.remaining() == 65 ? SenseSchemaValidation::partial_dynamic
                                                        : SenseSchemaValidation::mismatch;
}

} // namespace

SenseSchemaResult validate_sensor_sense_body(std::span<const std::byte> payload,
                                             std::size_t startBit,
                                             std::size_t bodyBits,
                                             std::uint8_t slotType) noexcept {
    Cursor cursor(payload, startBit, bodyBits);
    switch (slotType) {
    case 1:
        return {validate_type_1(cursor), 0};
    case 4:
        return {validate_type_4(cursor), 0};
    case 23:
        return validate_type_23(cursor);
    default:
        return {SenseSchemaValidation::unsupported, 0};
    }
}

} // namespace sunrise::server::bap::encrypted::activity_message
