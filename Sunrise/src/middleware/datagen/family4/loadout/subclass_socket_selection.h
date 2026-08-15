#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "../../../../state/account/account_state.h"
#include "../../../../state/build_data/socket_entry_lists/definition.h"
#include "../instance/instance_encoder.h"

namespace sunrise::middleware::datagen::family4::loadout {

/** Selector lanes one item instance publishes, one per semantic ability bucket. */
inline constexpr std::size_t kSelectorBucketCount = 12;
/** Selected socket entries per subclass: sprint, class, movement, grenade, super and melee. */
inline constexpr std::size_t kSelectedEntryCount = 6;

/** One selected socket entry and the semantic bucket it publishes into. */
struct SelectedEntry {
    std::uint8_t entry{};
    std::uint8_t bucket{};
};

/** The 6 entries one character has selected on its subclass. */
struct SubclassSelection {
    std::array<SelectedEntry, kSelectedEntryCount> selected{};
};

/**
 * Builds the selection for one subclass item. Only sprint is fixed; the other five entries belong
 * to that item, while the bucket the class ability publishes into follows its owning character.
 * @param subclass Per-instance subclass choices.
 * @param characterClass Class owning the subclass item.
 * @param output Receives the 6 selected entries.
 */
void subclass_selection(const state::account::inventory::SubclassState& subclass,
                        state::CharacterClass characterClass,
                        SubclassSelection& output) noexcept;

/**
 * Resolves one item's socket-entry states and selector lanes.
 * Only a list that carries a super lane belongs to a subclass, so every other item keeps its
 * absent and ready states and publishes no selector.
 * @param definition Installed socket-entry-list mapping.
 * @param subclass Per-instance subclass choices and acquired nodes.
 * @param characterClass Class owning the item.
 * @param output Receives the state of every fixed lane.
 * @param selectors Receives the selector lane of every semantic bucket.
 */
void resolve_socket_states(
    const state::build_data::socket_entry_lists::Definition& definition,
    const state::account::inventory::SubclassState& subclass,
    state::CharacterClass characterClass,
    std::array<instance::SocketEntryState, instance::layout::kSocketEntryStateCapacity>& output,
    std::array<instance::SocketSelector, kSelectorBucketCount>& selectors) noexcept;

} // namespace sunrise::middleware::datagen::family4::loadout
