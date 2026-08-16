#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>

#include "../../hooking/detour.h"

namespace sunrise::client::hooks::entity_spawn {

enum class Slot : std::size_t {
    create,
    allocateHandle,
    commit,
    allocateRecord,
    setRegistrationMode,
    count,
};

inline constexpr std::size_t kHookCount = static_cast<std::size_t>(Slot::count);

extern SRWLOCK g_lock;
extern std::array<hooking::detour::Handle, kHookCount> g_handles;

[[nodiscard]] void* create_entry_point() noexcept;
[[nodiscard]] void* allocate_handle_entry_point() noexcept;
[[nodiscard]] void* commit_entry_point() noexcept;
[[nodiscard]] void* allocate_record_entry_point() noexcept;
[[nodiscard]] void* set_registration_mode_entry_point() noexcept;

} // namespace sunrise::client::hooks::entity_spawn
