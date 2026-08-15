#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace sunrise::state::build_data::abilities {

/** A character record publishes 12 ability buckets with fixed meanings. */
inline constexpr std::size_t kBucketCapacity = 12;
/** Each bucket holds 16 definition hashes. */
inline constexpr std::size_t kBucketHashCapacity = 16;
/** The flat overflow bank holds 32 hashes no bucket category claims. */
inline constexpr std::size_t kOverflowCapacity = 32;
/**
 * Every selectable combination of all 9 installed subclasses. Three choices each for movement,
 * grenade and path plus two class abilities produce 54 rows per subclass, or 486 rows total.
 * The next power of two keeps this fixed storage bounded without imposing a protocol limit.
 */
inline constexpr std::size_t kDefinitionCapacity = 512;
/** All bits set marks a bucket no entry claimed. */
inline constexpr std::uint8_t kEmptyBucketKind = 0xFF;
/** Native selector entries are one byte, and shipped lists use fewer than 64. */
inline constexpr std::size_t kSelectorEntryCapacity = 64;

/** One ability bucket: the item category it collects and that category's definition hashes. */
struct Bucket {
    std::uint8_t kind{kEmptyBucketKind};
    std::uint8_t hashCount{};
    std::array<std::uint32_t, kBucketHashCapacity> hashes{};
};

/**
 * The 5 socket entries one character has selected on its subclass.
 * Sprint is the sixth selected entry but is not selectable, so it is fixed and not a key field.
 */
struct Selection {
    std::uint8_t movementEntry{};
    std::uint8_t grenadeEntry{};
    std::uint8_t superEntry{};
    std::uint8_t meleeEntry{};
    std::uint8_t classEntry{};

    friend bool operator==(const Selection&, const Selection&) = default;
};

/**
 * The ability buckets one subclass publishes under one ability selection.
 * Both key fields are needed. The socket entry list is the subclass's ability layout, and the
 * selection picks among the choices that layout offers.
 */
struct Definition {
    std::uint16_t socketEntryListIndex{};
    Selection selection{};
    /** One bit for every ability bucket whose item selector is authored by this row. */
    std::uint16_t selectorMask{};
    /** Socket-entry index published into each selected ability bucket. */
    std::array<std::uint8_t, kBucketCapacity> selectorEntries{};
    std::uint8_t overflowCount{};
    std::array<Bucket, kBucketCapacity> buckets{};
    std::array<std::uint32_t, kOverflowCapacity> overflow{};
};

} // namespace sunrise::state::build_data::abilities
