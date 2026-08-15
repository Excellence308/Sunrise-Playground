#include <array>

#include "../../../../middleware/content/packages/tables/ability_pool_reader.h"
#include "../../../../state/account/account_state.h"
#include "../../../../state/build_data/runtime.h"
#include "../../../../state/runtime/runtime.h"
#include "internal.h"

namespace sunrise::client::content::items::packages {
namespace {

namespace pool = middleware::content::packages::tables::abilities;
namespace domain = state::build_data::abilities;

/**
 * Socket entry of the sprint ability, the one entry the character cannot choose.
 * The other five come from the character's own selection.
 */
constexpr std::uint8_t kSprintEntry = 1;
/** Number of socket entries the character sheet's summary selects. */
constexpr std::size_t kSummaryEntryCount = 6;
/** Entry kind of the primary super, which stays active without a plug source of its own. */
constexpr std::uint8_t kSuperKind = 34;
/** A selector chain longer than this is a cycle, not a chain. */
constexpr std::size_t kSelectorChainLimit = 8;

/** Everything one subclass walk needs to read its entries and their pools. */
struct Walk {
    const reader::Source* source{};
    reader::Scratch* scratch{};
    std::vector<std::byte>* blob{};
    std::array<pool::Entry, pool::kEntryCapacity> entries{};
    std::size_t entryCount{};
    std::array<std::uint8_t, kSummaryEntryCount> selected{};
};

/**
 * Orders one character's selection the way the walk reads it.
 * When two entries share a group the first one claims it, so this order is fixed.
 * @param selection The character's 5 selected socket entries.
 * @return The 6 summary entries in claim order.
 */
[[nodiscard]] std::array<std::uint8_t, kSummaryEntryCount>
summary_entries(const domain::Selection& selection) noexcept {
    return {kSprintEntry,
            selection.classEntry,
            selection.movementEntry,
            selection.grenadeEntry,
            selection.superEntry,
            selection.meleeEntry};
}

/** Reads one entry's selected pool variant. */
[[nodiscard]] std::size_t records_of(const Walk& walk,
                                     const pool::Entry& entry,
                                     std::uint8_t subgroup,
                                     std::span<pool::PoolRecord> records) noexcept {
    if (entry.poolTag == 0
        || !reader::read_tag(*walk.source, *walk.scratch, entry.poolTag, *walk.blob)) {
        return 0;
    }
    return pool::read_pool_records(
        std::span<const std::byte>{*walk.blob}, entry, subgroup, records);
}

/** Follows one entry's selector chain to the bucket it lands in. */
[[nodiscard]] bool
selector_destination(const Walk& walk, std::uint8_t entryIndex, std::uint8_t& bucket) noexcept {
    std::uint8_t subgroup = 0;
    std::uint8_t element = 0;
    for (std::size_t step = 0; step < kSelectorChainLimit; ++step) {
        std::array<pool::PoolRecord, pool::kPoolRecordCapacity> records{};
        const std::size_t count =
            entryIndex < walk.entryCount
                ? records_of(walk, walk.entries[entryIndex], subgroup, records)
                : 0;
        if (element >= count) {
            return false;
        }
        const pool::PoolRecord& record = records[element];
        if (record.destination != pool::kEmptyByte) {
            bucket = record.destination;
            return bucket < domain::kBucketCapacity;
        }
        if (record.linkEntry == pool::kEmptyByte || record.linkSubgroup == pool::kEmptyByte
            || record.linkElement == pool::kEmptyByte) {
            return false;
        }
        entryIndex = record.linkEntry;
        subgroup = record.linkSubgroup;
        element = record.linkElement;
    }
    return false;
}

/** Chooses the active plug source of every entry group. */
void chosen_sources(const Walk& walk, std::array<std::uint32_t, 256>& sources) noexcept {
    sources.fill(pool::kNoPlugSource);
    for (const std::uint8_t entryIndex : walk.selected) {
        if (entryIndex >= walk.entryCount) {
            continue;
        }
        const pool::Entry& entry = walk.entries[entryIndex];
        if (entry.plugSource != pool::kNoPlugSource
            && sources[entry.group] == pool::kNoPlugSource) {
            sources[entry.group] = entry.plugSource;
        }
    }
}

/** @return True when an entry contributes its selected pool variant. */
[[nodiscard]] bool active(const pool::Entry& entry,
                          const std::array<std::uint32_t, 256>& sources) noexcept {
    if (entry.plugSource == pool::kNoPlugSource) {
        return entry.kind == kSuperKind;
    }
    return sources[entry.group] == entry.plugSource;
}

/** Assigns one selected entry's destination bucket, kind and item selector. */
[[nodiscard]] bool
assign_selected(const Walk& walk, std::uint8_t entryIndex, domain::Definition& output) noexcept {
    if (entryIndex >= walk.entryCount) {
        return false;
    }
    std::array<pool::PoolRecord, pool::kPoolRecordCapacity> records{};
    std::uint8_t bucket = 0;
    if (records_of(walk, walk.entries[entryIndex], 0, records) == 0
        || records[0].kind == pool::kEmptyByte || !selector_destination(walk, entryIndex, bucket)
        || output.buckets[bucket].kind != domain::kEmptyBucketKind) {
        return false;
    }
    output.buckets[bucket].kind = records[0].kind;
    output.selectorMask |= static_cast<std::uint16_t>(std::uint16_t{1} << bucket);
    output.selectorEntries[bucket] = entryIndex;
    return true;
}

/** Assigns every one of the six character-summary entries. */
[[nodiscard]] bool assign_selected_entries(const Walk& walk, domain::Definition& output) noexcept {
    for (const std::uint8_t entryIndex : walk.selected) {
        if (!assign_selected(walk, entryIndex, output)) {
            return false;
        }
    }
    return true;
}

/**
 * Replaces the primary super selector when the active tree declares another kind for its lane.
 * Later subclass trees keep the source-less primary super in the character summary, then expose
 * their replacement through an active entry whose selector chain reaches the same destination.
 */
[[nodiscard]] bool assign_active_super_selector(const Walk& walk,
                                                const std::array<std::uint32_t, 256>& sources,
                                                domain::Definition& output) noexcept {
    const std::uint8_t primaryEntry = output.selection.superEntry;
    std::uint8_t superBucket = 0;
    if (primaryEntry >= walk.entryCount || !selector_destination(walk, primaryEntry, superBucket)) {
        return false;
    }
    const std::uint8_t primaryKind = output.buckets[superBucket].kind;
    std::uint8_t replacementEntry = pool::kEmptyByte;
    std::uint8_t replacementKind = pool::kEmptyByte;
    for (std::size_t entryIndex = 0; entryIndex < walk.entryCount; ++entryIndex) {
        if (entryIndex == primaryEntry || !active(walk.entries[entryIndex], sources)) {
            continue;
        }
        std::array<pool::PoolRecord, pool::kPoolRecordCapacity> records{};
        if (records_of(walk, walk.entries[entryIndex], 0, records) == 0
            || records[0].kind == pool::kEmptyByte || records[0].kind == primaryKind) {
            continue;
        }
        std::uint8_t destination = 0;
        if (!selector_destination(walk, static_cast<std::uint8_t>(entryIndex), destination)
            || destination != superBucket) {
            continue;
        }
        const auto candidateEntry = static_cast<std::uint8_t>(entryIndex);
        if (replacementKind != pool::kEmptyByte
            && (replacementKind != records[0].kind || replacementEntry != candidateEntry)) {
            return false;
        }
        replacementEntry = candidateEntry;
        replacementKind = records[0].kind;
    }
    if (replacementKind != pool::kEmptyByte) {
        output.buckets[superBucket].kind = replacementKind;
        output.selectorEntries[superBucket] = replacementEntry;
    }
    return true;
}

/** @return True when one already-assigned bucket claims a category. */
[[nodiscard]] bool category_claimed(const domain::Definition& output,
                                    std::uint8_t category) noexcept {
    for (const domain::Bucket& bucket : output.buckets) {
        if (bucket.kind == category) {
            return true;
        }
    }
    return false;
}

/**
 * Adds authored destination lanes exposed only by the active tree's records.
 * Some later subclass trees publish supplemental lanes alongside their replacement super. A valid
 * hash record supplies both the bucket category and its destination, so no class, tree, entry or
 * ability hash is hardcoded here.
 */
void assign_active_destinations(const Walk& walk,
                                const std::array<std::uint32_t, 256>& sources,
                                domain::Definition& output) noexcept {
    for (std::size_t entryIndex = 0; entryIndex < walk.entryCount; ++entryIndex) {
        if (!active(walk.entries[entryIndex], sources)) {
            continue;
        }
        std::array<pool::PoolRecord, pool::kPoolRecordCapacity> records{};
        const std::size_t count = records_of(walk, walk.entries[entryIndex], 0, records);
        for (std::size_t index = 0; index < count; ++index) {
            const pool::PoolRecord& record = records[index];
            if (record.definitionHash == pool::kNoPlugSource || record.category == pool::kEmptyByte
                || record.destination == pool::kEmptyByte
                || record.destination >= output.buckets.size()
                || category_claimed(output, record.category)) {
                continue;
            }
            domain::Bucket& bucket = output.buckets[record.destination];
            if (bucket.kind != domain::kEmptyBucketKind) {
                continue;
            }
            bucket.kind = record.category;
            output.selectorMask |=
                static_cast<std::uint16_t>(std::uint16_t{1} << record.destination);
            output.selectorEntries[record.destination] = static_cast<std::uint8_t>(entryIndex);
        }
    }
}

/** Files one pool record's hash into the bucket its category names, or into overflow. */
void file_hash(const pool::PoolRecord& record, domain::Definition& output) noexcept {
    if (record.definitionHash == pool::kNoPlugSource) {
        return;
    }
    if (record.category == pool::kEmptyByte) {
        if (output.overflowCount < output.overflow.size()) {
            output.overflow[output.overflowCount++] = record.definitionHash;
        }
        return;
    }
    for (domain::Bucket& bucket : output.buckets) {
        if (bucket.kind == record.category && bucket.hashCount < bucket.hashes.size()) {
            bucket.hashes[bucket.hashCount++] = record.definitionHash;
        }
    }
}

} // namespace

/** Builds the ability buckets and item selectors one subclass publishes under one selection. */
bool build_ability_buckets(const reader::Source& source,
                           reader::Scratch& scratch,
                           std::span<const std::byte> listDefinition,
                           std::vector<std::byte>& blob,
                           const state::build_data::abilities::Selection& selection,
                           state::build_data::abilities::Definition& output) noexcept {
    Walk walk{};
    walk.source = &source;
    walk.scratch = &scratch;
    walk.blob = &blob;
    walk.entryCount = pool::read_entries(listDefinition, walk.entries);
    if (walk.entryCount == 0) {
        return false;
    }
    walk.selected = summary_entries(selection);

    for (domain::Bucket& bucket : output.buckets) {
        bucket = {};
    }
    output.selectorMask = 0;
    output.selectorEntries.fill(0);
    output.overflowCount = 0;
    if (!assign_selected_entries(walk, output)) {
        return false;
    }
    std::array<std::uint32_t, 256> sources{};
    chosen_sources(walk, sources);
    if (!assign_active_super_selector(walk, sources, output)) {
        return false;
    }
    assign_active_destinations(walk, sources, output);

    // Kinds must be complete before any hash is filed, because hashes route by category.
    for (std::size_t entryIndex = 0; entryIndex < walk.entryCount; ++entryIndex) {
        if (!active(walk.entries[entryIndex], sources)) {
            continue;
        }
        std::array<pool::PoolRecord, pool::kPoolRecordCapacity> records{};
        const std::size_t count = records_of(walk, walk.entries[entryIndex], 0, records);
        for (std::size_t entry = 0; entry < count; ++entry) {
            file_hash(records[entry], output);
        }
    }
    return true;
}

} // namespace sunrise::client::content::items::packages
