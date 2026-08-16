#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace sunrise::server::bap::encrypted::activity_message {

/** Structural result only; no sense-field value leaves the validator. */
enum class SenseSchemaValidation : std::uint8_t {
    unsupported,
    exact_grouped_gap,
    exact_grouped_gap_ambiguous,
    partial_dynamic,
    mismatch,
};

/** Payload-free result for one bounded native-schema walk. */
struct SenseSchemaResult final {
    SenseSchemaValidation validation{SenseSchemaValidation::unsupported};
    /** Bit N means the type-1 grouped-map shape closed with the opaque bit at gap N. */
    std::uint16_t type1GroupedGapMask{};
    /** Bit N means the type-23 grouped-map shape closed with the opaque bit at gap N. */
    std::uint8_t type23GroupedGapMask{};
};

/**
 * Validates one already-framed sensor-sense body against the recovered native schema shape.
 * The payload remains borrowed and no payload bits, values, hashes or copies are retained.
 * Type 1 tests ten positions for an opaque record bit within and around its grouped eight-marker
 * map. Type 23 tests eight positions for that bit within and around a grouped six-marker map. Each
 * path advances over only the marked values and returns only its structural match mask. Type 4
 * remains partial while its kind-0x22 branch is unresolved.
 */
[[nodiscard]] SenseSchemaResult
validate_sensor_sense_body(std::span<const std::byte> payload,
                           std::size_t startBit,
                           std::size_t bodyBits,
                           std::uint8_t slotType) noexcept;

} // namespace sunrise::server::bap::encrypted::activity_message
