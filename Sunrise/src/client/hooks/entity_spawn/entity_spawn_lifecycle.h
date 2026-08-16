#pragma once

namespace sunrise::client::hooks::entity_spawn {

/** Installs the optional, read-only static-object creation diagnostic. */
[[nodiscard]] bool install() noexcept;

/** Removes every static-object diagnostic detour in one transaction. */
[[nodiscard]] bool uninstall() noexcept;

/** @return True while all five diagnostic boundaries are attached. */
[[nodiscard]] bool is_installed() noexcept;

} // namespace sunrise::client::hooks::entity_spawn
