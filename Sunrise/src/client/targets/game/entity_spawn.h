#pragma once

#include <cstddef>

namespace sunrise::client::targets::game::entity_spawn {

/** Unowned game entry points used only by the static-object creation diagnostic. */
struct Targets {
    std::byte* create{};
    std::byte* allocateHandle{};
    std::byte* commit{};
    std::byte* allocateRecord{};
    std::byte* setRegistrationMode{};
};

/** Clears the published entity-spawn target group. */
void clear() noexcept;

/** @return Process-local entity-spawn target group. */
[[nodiscard]] const Targets& get() noexcept;

/** @return True after every diagnostic target is published. */
[[nodiscard]] bool is_resolved() noexcept;

} // namespace sunrise::client::targets::game::entity_spawn
