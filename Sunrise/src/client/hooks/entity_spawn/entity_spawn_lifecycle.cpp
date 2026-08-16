#include "entity_spawn_lifecycle.h"

#include <array>
#include <span>

#include "../../../core/logging/log.h"
#include "../../targets/game.h"
#include "entity_spawn_observer.h"

namespace sunrise::client::hooks::entity_spawn {

SRWLOCK g_lock{SRWLOCK_INIT};
std::array<hooking::detour::Handle, kHookCount> g_handles{};

namespace {

/** @return True when each slot in the all-or-nothing batch is attached. */
[[nodiscard]] bool all_attached() noexcept {
    for (const hooking::detour::Handle& handle : g_handles) {
        if (!handle.attached) {
            return false;
        }
    }
    return true;
}

} // namespace

/** Installs all diagnostic boundaries together, or leaves every boundary detached. */
bool install() noexcept {
    AcquireSRWLockExclusive(&g_lock);
    if (all_attached()) {
        ReleaseSRWLockExclusive(&g_lock);
        return true;
    }
    if (!targets::game::entity_spawn::is_resolved()) {
        ReleaseSRWLockExclusive(&g_lock);
        core::log::write(core::log::Channel::client,
                         core::log::Level::warn,
                         "ev=sobject_trace stage=install result=fail reason=target");
        return false;
    }

    const auto& targets = targets::game::entity_spawn::get();
    const std::array specs{
        hooking::detour::Spec{targets.create, create_entry_point()},
        hooking::detour::Spec{targets.allocateHandle, allocate_handle_entry_point()},
        hooking::detour::Spec{targets.commit, commit_entry_point()},
        hooking::detour::Spec{targets.allocateRecord, allocate_record_entry_point()},
        hooking::detour::Spec{targets.setRegistrationMode, set_registration_mode_entry_point()},
    };
    const bool installed = hooking::detour::install(specs, g_handles);
    ReleaseSRWLockExclusive(&g_lock);
    core::log::write(core::log::Channel::client,
                     installed ? core::log::Level::info : core::log::Level::warn,
                     installed ? "ev=sobject_trace stage=install result=ok"
                               : "ev=sobject_trace stage=install result=fail reason=detour");
    return installed;
}

/** Removes the complete diagnostic batch only while every replacement is idle. */
bool uninstall() noexcept {
    AcquireSRWLockExclusive(&g_lock);
    if (!all_attached()) {
        ReleaseSRWLockExclusive(&g_lock);
        return true;
    }
    const std::array protectedEntries{
        hooking::detour::ProtectedCodeEntry{create_entry_point()},
        hooking::detour::ProtectedCodeEntry{allocate_handle_entry_point()},
        hooking::detour::ProtectedCodeEntry{commit_entry_point()},
        hooking::detour::ProtectedCodeEntry{allocate_record_entry_point()},
        hooking::detour::ProtectedCodeEntry{set_registration_mode_entry_point()},
    };
    const bool removed = hooking::detour::uninstall(g_handles, protectedEntries)
                         == hooking::detour::UninstallResult::removed;
    ReleaseSRWLockExclusive(&g_lock);
    return removed;
}

/** @return True while the complete diagnostic batch is attached. */
bool is_installed() noexcept {
    AcquireSRWLockShared(&g_lock);
    const bool installed = all_attached();
    ReleaseSRWLockShared(&g_lock);
    return installed;
}

} // namespace sunrise::client::hooks::entity_spawn
