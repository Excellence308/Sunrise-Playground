#pragma once

#include <span>

#include "../../../patterns/registry.h"
#include "../entity_spawn.h"

namespace sunrise::client::targets::game::entity_spawn {

/**
 * Derives the optional entity-spawn diagnostic table.
 * @param image Executable ranges from the main game image.
 * @param output Receives all five entry points.
 * @return True only when every signature matches exactly once.
 */
[[nodiscard]] bool derive(std::span<const patterns::ImageRange> image, Targets& output) noexcept;

/** @param targets Fully validated entity-spawn table published without failure. */
void publish(const Targets& targets) noexcept;

} // namespace sunrise::client::targets::game::entity_spawn
