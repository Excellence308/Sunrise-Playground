#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "../../middleware/datagen/family4/loadout/loadout_resolver.h"
#include "../build_data/runtime.h"
#include "runtime.h"
#include "state_account_transaction_helpers.h"
#include "storage/internal.h"

namespace sunrise::state {

using namespace runtime::detail;
namespace authored_inventory = account::inventory;
namespace family4_loadout = middleware::datagen::family4::loadout;

/** Prepares one checked subclass socket-entry selection without publishing account State. */
bool prepare_subclass_selection(std::uint64_t subclassInstanceSoid,
                                std::uint8_t requestedEntry,
                                PendingSubclassSelection& mutation) noexcept {
    mutation = {};
    const AccountState snapshot = account_snapshot();
    std::size_t characterIndex = snapshot.characterCount;
    if (account::valid(snapshot)) {
        for (std::size_t index = 0; index < snapshot.characterCount; ++index) {
            if (snapshot.characters[index].selected) {
                characterIndex = index;
                break;
            }
        }
    }
    if (characterIndex >= snapshot.characterCount
        || !stage_subclass_selection(
            snapshot, characterIndex, subclassInstanceSoid, requestedEntry, mutation)) {
        report_subclass_selection("prepare",
                                  "fail",
                                  "selection_subclass_or_entry",
                                  0,
                                  subclassInstanceSoid,
                                  0,
                                  0,
                                  requestedEntry,
                                  0,
                                  build_data::socket_entry_lists::kNoEntryGroup,
                                  SubclassAbilityField::movement);
        mutation = {};
        return false;
    }
    report_subclass_selection("prepare",
                              "ok",
                              "ready",
                              mutation.characterSoid,
                              mutation.subclassInstanceSoid,
                              mutation.subclassDefinitionIndex,
                              mutation.socketEntryListIndex,
                              mutation.requestedEntry,
                              mutation.selectedEntry,
                              mutation.selectedGroup,
                              mutation.field);
    return true;
}

/** Produces the complete account after-image while the prepared subclass action remains current. */
bool preview_subclass_selection(const PendingSubclassSelection& mutation,
                                AccountState& after) noexcept {
    after = {};
    if (!mutation.prepared || mutation.accountSoid == 0 || mutation.characterSoid == 0
        || mutation.subclassInstanceSoid == 0 || mutation.characterIndex >= kCharacterCapacity
        || mutation.subclassDefinitionHash == authored_inventory::kNoDefinitionHash
        || mutation.requestedEntry >= build_data::socket_entry_lists::kEntryCapacity
        || mutation.selectedEntry >= build_data::socket_entry_lists::kEntryCapacity
        || mutation.selectedGroup == build_data::socket_entry_lists::kNoEntryGroup
        || mutation.selectedPlugSource == build_data::socket_entry_lists::kNoPlugSource) {
        return false;
    }
    const AccountState current = account_snapshot();
    if (mutation.characterIndex >= current.characterCount
        || current.primarySoid != mutation.accountSoid
        || !same_character(current.characters[mutation.characterIndex], mutation.beforeCharacter)) {
        return false;
    }
    PendingSubclassSelection canonical{};
    if (!stage_subclass_selection(current,
                                  mutation.characterIndex,
                                  mutation.subclassInstanceSoid,
                                  mutation.requestedEntry,
                                  canonical)
        || !same_subclass_transition(canonical, mutation)) {
        return false;
    }
    after = current;
    after.characters[mutation.characterIndex] = canonical.afterCharacter;
    family4_loadout::ResolvedLoadout resolved{};
    return account::valid(after)
           && family4_loadout::resolve(after, mutation.characterIndex, resolved);
}

/** Commits one prepared subclass selection behind exact account and character guards. */
bool commit_subclass_selection(PendingSubclassSelection& mutation) noexcept {
    const PendingSubclassSelection prepared = mutation;
    mutation = {};
    const auto fail = [&prepared](std::string_view reason) noexcept {
        report_subclass_selection("commit",
                                  "fail",
                                  reason,
                                  prepared.characterSoid,
                                  prepared.subclassInstanceSoid,
                                  prepared.subclassDefinitionIndex,
                                  prepared.socketEntryListIndex,
                                  prepared.requestedEntry,
                                  prepared.selectedEntry,
                                  prepared.selectedGroup,
                                  prepared.field);
        return false;
    };
    if (!prepared.prepared || prepared.accountSoid == 0 || prepared.characterSoid == 0
        || prepared.subclassInstanceSoid == 0 || prepared.characterIndex >= kCharacterCapacity
        || prepared.beforeCharacter.soid != prepared.characterSoid
        || prepared.afterCharacter.soid != prepared.characterSoid
        || prepared.subclassDefinitionHash == authored_inventory::kNoDefinitionHash
        || prepared.requestedEntry >= build_data::socket_entry_lists::kEntryCapacity
        || prepared.selectedEntry >= build_data::socket_entry_lists::kEntryCapacity
        || prepared.selectedGroup == build_data::socket_entry_lists::kNoEntryGroup
        || prepared.selectedPlugSource == build_data::socket_entry_lists::kNoPlugSource) {
        return fail("mutation");
    }

    report_subclass_selection("commit_begin",
                              "ok",
                              "ready",
                              prepared.characterSoid,
                              prepared.subclassInstanceSoid,
                              prepared.subclassDefinitionIndex,
                              prepared.socketEntryListIndex,
                              prepared.requestedEntry,
                              prepared.selectedEntry,
                              prepared.selectedGroup,
                              prepared.field);
    AcquireSRWLockExclusive(&runtime::storage::g_stateLock);
    AccountState candidate = runtime::storage::g_state.account;
    if (prepared.characterIndex >= candidate.characterCount
        || candidate.primarySoid != prepared.accountSoid
        || !same_character(candidate.characters[prepared.characterIndex],
                           prepared.beforeCharacter)) {
        ReleaseSRWLockExclusive(&runtime::storage::g_stateLock);
        return fail("stale");
    }
    PendingSubclassSelection canonical{};
    if (!stage_subclass_selection(candidate,
                                  prepared.characterIndex,
                                  prepared.subclassInstanceSoid,
                                  prepared.requestedEntry,
                                  canonical)
        || !same_subclass_transition(canonical, prepared)) {
        ReleaseSRWLockExclusive(&runtime::storage::g_stateLock);
        return fail("transition");
    }
    candidate.characters[prepared.characterIndex] = canonical.afterCharacter;
    family4_loadout::ResolvedLoadout checked{};
    if (!account::valid(candidate)
        || !family4_loadout::resolve(candidate, prepared.characterIndex, checked)) {
        ReleaseSRWLockExclusive(&runtime::storage::g_stateLock);
        return fail("account_or_resolve");
    }
    runtime::storage::g_state.account = candidate;
    ReleaseSRWLockExclusive(&runtime::storage::g_stateLock);

    report_subclass_selection("commit_end",
                              "ok",
                              "published",
                              prepared.characterSoid,
                              prepared.subclassInstanceSoid,
                              prepared.subclassDefinitionIndex,
                              prepared.socketEntryListIndex,
                              prepared.requestedEntry,
                              prepared.selectedEntry,
                              prepared.selectedGroup,
                              prepared.field);
    return true;
}


} // namespace sunrise::state
