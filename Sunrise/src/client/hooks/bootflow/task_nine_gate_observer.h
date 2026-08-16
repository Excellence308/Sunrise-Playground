#pragma once

namespace sunrise::client::hooks::bootflow {

/** Attaches the pass-through initial-slice task-9 and nested-helper diagnostic. */
[[nodiscard]] bool install_task_nine_gate_observer() noexcept;

/** Detaches the pass-through initial-slice task-9 and nested-helper diagnostic. */
void uninstall_task_nine_gate_observer() noexcept;

} // namespace sunrise::client::hooks::bootflow
