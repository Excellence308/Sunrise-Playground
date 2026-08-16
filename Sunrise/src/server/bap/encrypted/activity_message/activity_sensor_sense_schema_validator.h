#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace sunrise::server::bap::encrypted::activity_message {

/** Structural result only; no sense-field value leaves the validator. */
enum class SenseSchemaValidation : std::uint8_t {
    unsupported,
    exact_record_prefix_map,
    exact_record_trailer_map,
    exact_map_ambiguous,
    partial_dynamic,
    mismatch,
};

/**
 * Validates one already-framed sensor-sense body against the recovered native schema shape.
 * The payload remains borrowed and no payload bits, values, hashes or copies are retained.
 * Types 1 and 23 test the two bounded placements of an opaque record bit around a grouped
 * optional-field presence map. Type 4 remains partial while its kind-0x22 branch is unresolved.
 */
[[nodiscard]] SenseSchemaValidation
validate_sensor_sense_body(std::span<const std::byte> payload,
                           std::size_t startBit,
                           std::size_t bodyBits,
                           std::uint8_t slotType) noexcept;

} // namespace sunrise::server::bap::encrypted::activity_message
