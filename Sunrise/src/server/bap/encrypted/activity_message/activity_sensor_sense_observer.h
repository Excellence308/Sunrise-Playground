#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace sunrise::server::bap::encrypted::activity_message {

/** Client-to-host sensor sense update. Upstream currently accepts it as a one-way no-op. */
inline constexpr std::uint32_t kSensorSenseMessageType = 6;

/**
 * Frames exact roster records under the two Trophy Hall group keys and validates recovered schemas.
 * The sensitive payload is inspected in place and is never copied, hashed, logged or retained.
 * @param payload Borrowed type-6 body owned by the request decoder.
 */
void observe_sensor_sense_structure(std::span<const std::byte> payload) noexcept;

} // namespace sunrise::server::bap::encrypted::activity_message
