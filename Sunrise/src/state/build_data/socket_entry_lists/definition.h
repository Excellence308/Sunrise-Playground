#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace sunrise::state::build_data::socket_entry_lists {

/** Signed native definition indices give 32,768 nonnegative socket-list rows. */
inline constexpr std::size_t kDefinitionCapacity = 32768;
/** One item-instance record has 36 socket-entry state lanes. */
inline constexpr std::size_t kEntryCapacity = 36;
/** Native state 0 marks a socket entry whose plug source is absent. */
inline constexpr std::uint8_t kAbsentEntryState = 0;
/** Native state 16 marks an initial socket entry whose plug source is available. */
inline constexpr std::uint8_t kReadyEntryState = 16;
/** Native state 17 marks an acquired socket entry that is not currently selected. */
inline constexpr std::uint8_t kAcquiredEntryState = 17;
/** Native state 18 marks a socket entry the character has selected. */
inline constexpr std::uint8_t kActiveEntryState = 18;
/** Entry kind of the super lane, which is active although it carries no plug source. */
inline constexpr std::uint8_t kSuperEntryKind = 34;
/** Group value of an entry that competes in no bucket. */
inline constexpr std::uint8_t kNoEntryGroup = 0xFF;
/** Plug source of an entry that has none. */
inline constexpr std::uint32_t kNoPlugSource = 0x811C9DC5U;

/** One socket entry's competition group, kind and plug source. */
struct Entry {
    std::uint32_t plugSource{};
    std::uint8_t group{kNoEntryGroup};
    std::uint8_t kind{};
};

/** One installed-build socket-entry-list identity and its safe initial state mask. */
struct Definition {
    std::uint32_t definitionHash{};
    std::uint16_t definitionIndex{};
    std::uint8_t entryCount{};
    std::uint64_t readyMask{};
};

/** List index of an unused entry-table row. */
inline constexpr std::uint16_t kNoEntryTable = 0xFFFF;

/**
 * Per-entry selection inputs, kept only for lists that carry a super lane. Those are the
 * subclasses, the only items whose sockets a character selects. Holding this on every definition
 * would cost 10 MB of fixed storage to serve 9 items.
 */
struct EntryTable {
    std::uint16_t definitionIndex{kNoEntryTable};
    std::array<Entry, kEntryCapacity> entries{};
};

/**
 * Resolves the unique source-less kind-34 entry used by the character summary.
 * @param definition Installed socket-entry-list mapping.
 * @param table Per-entry inputs for the same list.
 * @param output Receives the primary super selector.
 * @return True when the table contains exactly one primary super entry.
 */
[[nodiscard]] inline bool primary_super_entry(const Definition& definition,
                                              const EntryTable& table,
                                              std::uint8_t& output) noexcept {
    output = static_cast<std::uint8_t>(kEntryCapacity);
    if (table.definitionIndex != definition.definitionIndex
        || definition.entryCount > kEntryCapacity) {
        return false;
    }

    std::size_t primary = definition.entryCount;
    for (std::size_t entry = 0; entry < definition.entryCount; ++entry) {
        const Entry& candidate = table.entries[entry];
        if (candidate.plugSource != kNoPlugSource || candidate.kind != kSuperEntryKind) {
            continue;
        }
        if (primary != definition.entryCount) {
            return false;
        }
        primary = entry;
    }
    if (primary == definition.entryCount) {
        return false;
    }
    output = static_cast<std::uint8_t>(primary);
    return true;
}

/** Lists that may carry a super lane. Only 9 subclasses ship, so this is plenty. */
inline constexpr std::size_t kEntryTableCapacity = 32;

} // namespace sunrise::state::build_data::socket_entry_lists
