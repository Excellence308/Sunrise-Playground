#include <array>

#include "../../../../state/account/account_state.h"
#include "../../../../state/build_data/runtime.h"
#include "../../../../state/runtime/runtime.h"
#include "internal.h"

namespace sunrise::client::content::items::packages {
namespace {

namespace domain = state::build_data::abilities;
namespace lists = state::build_data::socket_entry_lists;

/** @param rows Rows built so far. @return True when the candidate's key is already held. */
[[nodiscard]] bool held(std::span<const domain::Definition> rows,
                        const domain::Definition& row) noexcept {
    for (const domain::Definition& existing : rows) {
        if (existing.socketEntryListIndex == row.socketEntryListIndex
            && existing.selection == row.selection) {
            return true;
        }
    }
    return false;
}

/** One representative entry for every distinct plug source in one semantic ability group. */
struct EntryChoices {
    std::array<std::uint8_t, lists::kEntryCapacity> entries{};
    std::size_t count{};
};

/** Collects canonical selectable alternatives from the group of one configured entry. */
[[nodiscard]] bool choices_of(const lists::Definition& definition,
                              const lists::EntryTable& table,
                              std::uint8_t selected,
                              EntryChoices& output) noexcept {
    output = {};
    if (selected >= definition.entryCount) {
        return false;
    }
    const lists::Entry& current = table.entries[selected];
    if (current.plugSource == lists::kNoPlugSource) {
        if (current.kind != lists::kSuperEntryKind) {
            return false;
        }
        output.entries[0] = selected;
        output.count = 1;
        return true;
    }
    if (current.group == lists::kNoEntryGroup) {
        return false;
    }

    bool containsCurrentSource = false;
    for (std::size_t entry = 0; entry < definition.entryCount; ++entry) {
        const std::uint64_t bit = std::uint64_t{1} << entry;
        const lists::Entry& candidate = table.entries[entry];
        if ((definition.readyMask & bit) == 0 || candidate.group != current.group
            || candidate.plugSource == lists::kNoPlugSource) {
            continue;
        }
        bool duplicate = false;
        for (std::size_t heldIndex = 0; heldIndex < output.count; ++heldIndex) {
            duplicate =
                duplicate
                || table.entries[output.entries[heldIndex]].plugSource == candidate.plugSource;
        }
        if (!duplicate) {
            if (output.count >= output.entries.size()) {
                return false;
            }
            output.entries[output.count++] = static_cast<std::uint8_t>(entry);
        }
        containsCurrentSource = containsCurrentSource || candidate.plugSource == current.plugSource;
    }
    return output.count != 0 && containsCurrentSource;
}

/** Collects every authored representative of one already-selected plug source. */
[[nodiscard]] bool representatives_of(const lists::Definition& definition,
                                      const lists::EntryTable& table,
                                      std::uint8_t selected,
                                      EntryChoices& output) noexcept {
    output = {};
    if (selected >= definition.entryCount) {
        return false;
    }
    const lists::Entry& current = table.entries[selected];
    if (current.plugSource == lists::kNoPlugSource) {
        if (current.kind != lists::kSuperEntryKind) {
            return false;
        }
        output.entries[0] = selected;
        output.count = 1;
        return true;
    }
    if (current.group == lists::kNoEntryGroup) {
        return false;
    }
    for (std::size_t entry = 0; entry < definition.entryCount; ++entry) {
        const lists::Entry& candidate = table.entries[entry];
        if ((definition.readyMask & (std::uint64_t{1} << entry)) == 0
            || candidate.group != current.group || candidate.plugSource != current.plugSource) {
            continue;
        }
        if (output.count >= output.entries.size()) {
            return false;
        }
        output.entries[output.count++] = static_cast<std::uint8_t>(entry);
    }
    return output.count != 0;
}

/** Resolves one plug-source combination through whichever authored representatives are valid. */
[[nodiscard]] bool build_supported_row(const reader::Source& source,
                                       reader::Scratch& scratch,
                                       std::span<const std::byte> definitionBytes,
                                       std::vector<std::byte>& blob,
                                       const lists::Definition& definition,
                                       const lists::EntryTable& table,
                                       std::uint16_t socketEntryListIndex,
                                       const domain::Selection& sourceSelection,
                                       domain::Definition& output) noexcept {
    const std::array<std::uint8_t, 5> selected{
        sourceSelection.movementEntry,
        sourceSelection.grenadeEntry,
        sourceSelection.superEntry,
        sourceSelection.meleeEntry,
        sourceSelection.classEntry,
    };
    std::array<EntryChoices, 5> representatives{};
    for (std::size_t field = 0; field < selected.size(); ++field) {
        if (!representatives_of(definition, table, selected[field], representatives[field])) {
            return false;
        }
    }

    for (std::size_t movement = 0; movement < representatives[0].count; ++movement) {
        for (std::size_t grenade = 0; grenade < representatives[1].count; ++grenade) {
            for (std::size_t super = 0; super < representatives[2].count; ++super) {
                for (std::size_t melee = 0; melee < representatives[3].count; ++melee) {
                    for (std::size_t classAbility = 0; classAbility < representatives[4].count;
                         ++classAbility) {
                        domain::Definition candidate{};
                        candidate.socketEntryListIndex = socketEntryListIndex;
                        candidate.selection = {
                            representatives[0].entries[movement],
                            representatives[1].entries[grenade],
                            representatives[2].entries[super],
                            representatives[3].entries[melee],
                            representatives[4].entries[classAbility],
                        };
                        if (build_ability_buckets(source,
                                                  scratch,
                                                  definitionBytes,
                                                  blob,
                                                  candidate.selection,
                                                  candidate)) {
                            output = candidate;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

/** Builds every cross-product selection the configured subclass exposes. */
[[nodiscard]] bool build_subclass_abilities(const reader::Source& source,
                                            reader::Scratch& scratch,
                                            std::span<const std::byte> definitionBytes,
                                            std::vector<std::byte>& blob,
                                            std::uint16_t socketEntryListIndex,
                                            const domain::Selection& configured,
                                            std::span<domain::Definition> output,
                                            std::size_t& count) noexcept {
    lists::Definition definition{};
    lists::EntryTable table{};
    if (!state::build_data::find_socket_entry_list(socketEntryListIndex, definition)
        || !state::build_data::find_socket_entry_table(socketEntryListIndex, table)
        || definition.definitionIndex != socketEntryListIndex
        || table.definitionIndex != socketEntryListIndex) {
        return false;
    }

    std::array<EntryChoices, 5> choices{};
    const std::array<std::uint8_t, 5> selected{
        configured.movementEntry,
        configured.grenadeEntry,
        configured.superEntry,
        configured.meleeEntry,
        configured.classEntry,
    };
    for (std::size_t field = 0; field < selected.size(); ++field) {
        if (field == 2) {
            continue;
        }
        if (!choices_of(definition, table, selected[field], choices[field])) {
            return false;
        }
    }
    // Movement, grenade, melee path and class ability must be independent groups. Super is the
    // one source-less lane and is deliberately excluded from this check.
    const std::array<std::size_t, 4> selectable{0, 1, 3, 4};
    for (std::size_t field = 0; field < selectable.size(); ++field) {
        const std::uint8_t group = table.entries[selected[selectable[field]]].group;
        if (group == lists::kNoEntryGroup) {
            return false;
        }
        for (std::size_t other = field + 1; other < selectable.size(); ++other) {
            if (group == table.entries[selected[selectable[other]]].group) {
                return false;
            }
        }
    }

    for (std::size_t movement = 0; movement < choices[0].count; ++movement) {
        for (std::size_t grenade = 0; grenade < choices[1].count; ++grenade) {
            for (std::size_t melee = 0; melee < choices[3].count; ++melee) {
                const std::uint8_t meleeEntry = choices[3].entries[melee];
                std::uint8_t superEntry = 0;
                if (!lists::primary_super_entry(definition, table, superEntry)) {
                    return false;
                }
                for (std::size_t classAbility = 0; classAbility < choices[4].count;
                     ++classAbility) {
                    domain::Definition row{};
                    row.socketEntryListIndex = socketEntryListIndex;
                    row.selection = {
                        choices[0].entries[movement],
                        choices[1].entries[grenade],
                        superEntry,
                        meleeEntry,
                        choices[4].entries[classAbility],
                    };
                    if (count >= output.size()) {
                        return false;
                    }
                    // A shared plug source can span several authored nodes. Its first node is not
                    // necessarily the one the character summary uses, so resolve the
                    // representative from the installed selector chains instead of assuming.
                    if (!build_supported_row(source,
                                             scratch,
                                             definitionBytes,
                                             blob,
                                             definition,
                                             table,
                                             socketEntryListIndex,
                                             row.selection,
                                             row)
                        || held(output.first(count), row)) {
                        continue;
                    }
                    output[count++] = row;
                }
            }
        }
    }
    return true;
}

/** @return The native first choice in each selectable group, shared by all shipped subclasses. */
[[nodiscard]] constexpr domain::Selection default_selection() noexcept {
    return {state::kDefaultMovementAbilityEntry,
            state::kDefaultGrenadeAbilityEntry,
            state::kDefaultSuperAbilityEntry,
            state::kDefaultMeleeAbilityEntry,
            state::kDefaultClassAbilityEntry};
}

} // namespace

/** Builds every ability combination for every installed list identified as a subclass. */
bool build_character_abilities(const reader::Source& source,
                               reader::Scratch& scratch,
                               std::span<const std::byte> root,
                               std::vector<std::byte>& table,
                               std::vector<std::byte>& definition,
                               std::vector<std::byte>& blob,
                               std::span<state::build_data::abilities::Definition> output,
                               std::size_t& count) noexcept {
    count = 0;
    std::uint32_t tableTag = 0;
    tables::Array rows{};
    if (!tables::slot_tag(root, tables::kSocketEntryListTableSlot, tableTag) || tableTag == 0
        || !reader::read_tag(source, scratch, tableTag, table)
        || !tables::find_array_at(
            std::span<const std::byte>{table}, tables::kTableArrayDescriptor, rows)) {
        return false;
    }
    const std::size_t listCount = state::build_data::socket_entry_list_count();
    for (std::size_t list = 0; list < listCount && count < output.size(); ++list) {
        const auto socketEntryListIndex = static_cast<std::uint16_t>(list);
        lists::EntryTable entryTable{};
        if (!state::build_data::find_socket_entry_table(socketEntryListIndex, entryTable)) {
            continue;
        }
        tables::IndexRow indexRow{};
        if (!tables::index_row(
                std::span<const std::byte>{table}, rows, socketEntryListIndex, indexRow)
            || indexRow.targetTag == 0
            || !reader::read_tag(source, scratch, indexRow.targetTag, definition)
            || !build_subclass_abilities(source,
                                         scratch,
                                         std::span<const std::byte>{definition},
                                         blob,
                                         socketEntryListIndex,
                                         default_selection(),
                                         output,
                                         count)) {
            return false;
        }
    }
    return true;
}

} // namespace sunrise::client::content::items::packages
