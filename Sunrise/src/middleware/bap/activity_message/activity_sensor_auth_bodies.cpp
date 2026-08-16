#include "sensor_auth_update.h"

namespace sunrise::middleware::bap::activity_message::sensor_auth_update {
namespace {

namespace bits = encoding::bits;

/** Slot types whose auth body this module fills. Every other block is seed-only. */
/** Auth schema `0x80807EC9`: optional state plus two biased mode fields. */
constexpr std::uint8_t kSlotType1 = 1;
/** Auth schema `0x80807DA1`: one optional field, two modes, a bool and four optionals. */
constexpr std::uint8_t kSlotType2 = 2;
/** Auth schema `0x8080992F`: transform, neutral quaternion and absent polymorphic tail. */
constexpr std::uint8_t kSlotType4 = 4;
/** Auth schema `0x80804F04`: two u64s, one byte and three nested neutral records. */
constexpr std::uint8_t kSlotType5 = 5;
constexpr std::uint8_t kSlotTypeParticipation = 13;
constexpr std::uint8_t kSlotTypeLifetime = 17;
constexpr std::uint8_t kSlotTypeConfiguration = 8;
constexpr std::uint8_t kSlotTypePackage = 16;
/** Auth schema `0x80809919`: shared activity record, bool, then signed i32. */
constexpr std::uint8_t kSlotType18 = 18;
/** Auth schema `0x80804F48`: three neutral 32-bit/16-bit/bool tuples. */
constexpr std::uint8_t kSlotType23 = 23;
/** Auth schema `0x808099BF`: two bools, two biased 2-bit values, then the shared record. */
constexpr std::uint8_t kSlotType35 = 35;
constexpr std::uint8_t kSlotTypeQueues = 41;
constexpr std::uint8_t kSlotTypeSpawnKeys = 67;
/** Auth schema `0x808094F1`: a 5-bit value, two optionals and one signed i16. */
constexpr std::uint8_t kSlotType70 = 70;

/** Body widths, each checked against the writer after the body is written. */
constexpr std::size_t kType1Bits = 24;
constexpr std::size_t kType2Bits = 11;
constexpr std::size_t kType4Bits = 253;
constexpr std::size_t kType5Bits = 278;
constexpr std::size_t kParticipationBits = 192;
constexpr std::size_t kParticipationRegionBits = 32;
constexpr std::size_t kLifetimeBits = 520;
constexpr std::size_t kConfigurationBits = 35;
constexpr std::size_t kPackageBits = 7;
constexpr std::size_t kType18Bits = 386;
constexpr std::size_t kType23Bits = 147;
constexpr std::size_t kType35Bits = 359;
constexpr std::size_t kQueueBits = 12;
constexpr std::size_t kSpawnKeyBits = 32 * 32 + 1 + 32;
constexpr std::size_t kType70Bits = 23;

/** Signed fields in these bodies carry a -2^31 bias, so this wire value stores zero. */
constexpr std::uint32_t kSignedZero = 0x80000000;
/** Signed 16-bit fields use the same bias at their own width. */
constexpr std::uint32_t kSigned16Zero = 0x8000;
/** The same bias wraps at the top of the field, so this wire value stores -1. */
constexpr std::uint32_t kSignedMinusOne = 0x7FFFFFFF;
/** The region index rides the same bias, so its wire value is the bias plus the index. */
constexpr std::uint32_t kRegionBias = 0x80000000;
/** Message 52's team-state byte 1, where bit 1 is `awaiting_client_sync`. */
constexpr std::uint32_t kAwaitingClientSync = 2;
/** Neutral runtime-i32 override that forces the type-17 waiting selector to zero. */
constexpr std::uint32_t kWaitingSwitchKey = 0xB3C1251B;
constexpr std::uint32_t kWaitingSwitchClass = 0x80800007;
/** Type 17 carries 3 spawn overrides. Wire zero stores index -1 and disables one. */
constexpr std::size_t kSpawnOverrideCount = 3;
constexpr std::uint8_t kSpawnOverrideIndexWidth = 10;
constexpr std::uint32_t kSpawnOverrideIndexBias = 1;
/** Type 67 maps the 32 spawn-key ordinals to themselves, matching its constructor. */
constexpr std::size_t kSpawnKeyCount = 32;

/**
 * Writes auth schema `0x808099C4`, shared by slot types 18 and 35.
 * The native schema has one bool, five unsigned 64-bit values, and one unsigned 32-bit value.
 * Its constructed state is all zeroes.
 * @param writer Body writer.
 * @return True when all 353 bits fit.
 */
[[nodiscard]] bool write_common_activity_record(bits::Writer& writer) noexcept {
    bool encoded = writer.write(0, kPresenceWidth);
    for (std::size_t field = 0; encoded && field < 5; ++field) {
        encoded = writer.write(0, 64);
    }
    return encoded && writer.write(0, 32);
}

/**
 * Writes schema `0x80809C42`, reused by the captured type-5 auth tree.
 * Its fields are an unsigned i32, a bias-1 seven-bit value and a signed i16.
 * @param writer Body writer.
 * @return True when the 55-bit neutral record fits.
 */
[[nodiscard]] bool write_schema_80809c42(bits::Writer& writer) noexcept {
    return writer.write(0, 32) && writer.write(1, 7) && writer.write(kSigned16Zero, 16);
}

/** Writes auth schema `0x80807EC9` in its constructed neutral state. */
[[nodiscard]] bool write_type1(bits::Writer& writer) noexcept {
    // The first 18 fields and the last field are optional and absent. The two mandatory mode
    // fields between them are bias-1 values with widths two and three.
    return pad_bits(writer, 18) && writer.write(1, 2) && writer.write(1, 3)
           && writer.write(0, kPresenceWidth);
}

/** Writes auth schema `0x80807DA1` in its constructed neutral state. */
[[nodiscard]] bool write_type2(bits::Writer& writer) noexcept {
    // One optional, two bias-1 modes, one bool, then four optional nested records.
    return writer.write(0, kPresenceWidth) && writer.write(1, 2) && writer.write(1, 3)
           && writer.write(0, kPresenceWidth) && pad_bits(writer, 4);
}

/** Writes auth schema `0x8080992F` in its constructed neutral state. */
[[nodiscard]] bool write_type4(bits::Writer& writer) noexcept {
    // The runtime's raw-quaternion mode reads x/y/z as three 32-bit floats and synthesizes w=1.
    // Schema 0x80809AEA then carries an unbiased two-bit value. Its nested kind-0x22 field first
    // decodes schema 0x80800046; a clear kind-0x17 presence bit stores -1 and omits the object.
    return writer.write(kSignedZero, 32) && writer.write(kSignedZero, 32)
           && writer.write(0, kPresenceWidth) && writer.write(0, kPresenceWidth)
           && writer.write(kSignedZero, 32) && write_schema_80809c42(writer)
           && pad_bits(writer, 96) && writer.write(0, kPresenceWidth)
           && writer.write(0, 2) && writer.write(0, kPresenceWidth);
}

/** Writes auth schema `0x80804F04` in its constructed neutral state. */
[[nodiscard]] bool write_type5(bits::Writer& writer) noexcept {
    // The first nested branch is u32 followed by schema 0x80809C42. The final field is another
    // 0x80809C42 record. None of these fields is optional.
    return writer.write(0, 64) && writer.write(0, 64) && writer.write(0, 8)
           && writer.write(0, 32) && write_schema_80809c42(writer)
           && write_schema_80809c42(writer);
}

/** Writes auth schema `0x80804F48` in its constructed neutral state. */
[[nodiscard]] bool write_type23(bits::Writer& writer) noexcept {
    bool encoded = true;
    for (std::size_t tuple = 0; encoded && tuple < 3; ++tuple) {
        encoded = writer.write(0, 32) && writer.write(kSigned16Zero, 16)
                  && writer.write(0, kPresenceWidth);
    }
    return encoded;
}

/** Writes auth schema `0x808094F1` in its constructed neutral state. */
[[nodiscard]] bool write_type70(bits::Writer& writer) noexcept {
    return writer.write(0, 5) && writer.write(0, kPresenceWidth)
           && writer.write(kSigned16Zero, 16) && writer.write(0, kPresenceWidth);
}

/**
 * Writes the participation body, which binds the player and latches the region. Zero-fill is not
 * safe here. Every biased field must carry its bias, or a stored zero decodes to the smallest
 * signed value.
 * @param writer Body writer.
 * @param snapshot Message input.
 * @return True when the body fits.
 */
[[nodiscard]] bool write_participation(bits::Writer& writer, const Snapshot& snapshot) noexcept {
    // An optional field's value follows its presence bit, so sending +0 shifts everything below.
    bool encoded = writer.write(snapshot.hasRegion ? 1U : 0U, kPresenceWidth);
    if (encoded && snapshot.hasRegion) {
        encoded = writer.write(kRegionBias + snapshot.region, kParticipationRegionBits);
    }
    // The participation record is this body's head, so struct +8 and +10 are record +8 and +10.
    // Record +8 is step 36 task 9's own term and +10 is the spawn gate's.
    return encoded && writer.write(0, kPresenceWidth) && writer.write(1, kPresenceWidth)
           && writer.write(1, kPresenceWidth) && writer.write(1, kPresenceWidth)
           && writer.write(0, kPresenceWidth) && writer.write(1, 3) && writer.write(1, 2)
           && writer.write(0, 3) && writer.write(0, 32) && writer.write(1, 5)
           && writer.write(0, kPresenceWidth) && writer.write(0, 3)
           && writer.write(1, kPresenceWidth) && writer.write(snapshot.playerKey, 64)
           && writer.write(0, 5) && writer.write(3, 6) && writer.write(0, 6)
           && writer.write(0, 6)
           // Byte 736 skips the respawn delay, whose countdown never expires when the content
           // delay is negative. Byte 737 holds the spawn while the client loads.
           && writer.write(1, kPresenceWidth)
           && writer.write(snapshot.awaitClientSync ? kAwaitingClientSync : 0U, 4)
           && writer.write(0, 3) && writer.write(0, kPresenceWidth) && writer.write(128, 8)
           && writer.write(kSignedZero, 32);
}

/**
 * Writes the lifetime body, which is the activity state the roster reports.
 * @param writer Body writer.
 * @param snapshot Message input.
 * @return True when the body fits.
 */
[[nodiscard]] bool write_lifetime(bits::Writer& writer, const Snapshot& snapshot) noexcept {
    bool encoded = writer.write(std::uint32_t{snapshot.lifetime} + 1, 4) && writer.write(1, 3)
                   && writer.write(0, kPresenceWidth) && writer.write(kSignedZero, 32)
                   && writer.write(0, 32) && writer.write(kSignedZero, 32) && writer.write(1, 6)
                   && writer.write(kWaitingSwitchKey, 32) && writer.write(1, kPresenceWidth)
                   && writer.write(kWaitingSwitchClass, 32) && writer.write(kSignedZero, 32)
                   && writer.write(kSignedZero, 32);
    for (std::size_t index = 0; encoded && index < kSpawnOverrideCount; ++index) {
        const std::uint32_t slice =
            snapshot.hasSpawnOverride ? snapshot.spawnSliceSet + kSpawnOverrideIndexBias : 0U;
        const std::uint32_t hash =
            snapshot.hasSpawnOverride ? snapshot.spawnSetHash : kAbsentSpawnSetHash;
        encoded = writer.write(slice, kSpawnOverrideIndexWidth) && writer.write(hash, 32);
    }
    // Struct `+1256` is the out-of-bounds `activity_quarantine` selector. The reader arms the
    // quarantine at or below 0x3F unsigned, so minus one leaves it clear and teleports nobody.
    return encoded && writer.write(0, kPresenceWidth) && writer.write(0, 32)
           && writer.write(kSignedMinusOne, 32) && writer.write(0, 32)
           && writer.write(kSlotTypeBias, kSlotTypeWidth)
           && writer.write(kSlotIndexBias, kSlotIndexWidth) && writer.write(0, 32)
           && writer.write(0, 3);
}

/**
 * Writes the spawn-key body, which maps the 32 ordinals to themselves.
 * @param writer Body writer.
 * @return True when the body fits.
 */
[[nodiscard]] bool write_spawn_keys(bits::Writer& writer) noexcept {
    bool encoded = true;
    for (std::size_t index = 0; encoded && index < kSpawnKeyCount; ++index) {
        encoded = writer.write(kSignedZero + index, 32);
    }
    return encoded && writer.write(0, kPresenceWidth) && writer.write(kSignedMinusOne, 32);
}

} // namespace

/** Reports how many bits of auth body one slot carries. */
std::size_t
auth_body_bits(const Snapshot& snapshot, std::uint8_t slotType, bool carriesPlayerKey) noexcept {
    if (slotType == kSlotType1) {
        return kType1Bits;
    }
    if (slotType == kSlotType2) {
        return kType2Bits;
    }
    if (slotType == kSlotType4) {
        return kType4Bits;
    }
    if (slotType == kSlotType5) {
        return kType5Bits;
    }
    if (slotType == kSlotTypeParticipation) {
        return carriesPlayerKey
                   ? kParticipationBits + (snapshot.hasRegion ? kParticipationRegionBits : 0)
                   : 0;
    }
    if (slotType == kSlotTypeLifetime) {
        return kLifetimeBits;
    }
    if (slotType == kSlotTypeConfiguration) {
        return kConfigurationBits;
    }
    if (slotType == kSlotTypePackage) {
        return kPackageBits;
    }
    if (slotType == kSlotType18) {
        return kType18Bits;
    }
    if (slotType == kSlotType23) {
        return kType23Bits;
    }
    if (slotType == kSlotType35) {
        return kType35Bits;
    }
    if (slotType == kSlotTypeQueues) {
        return kQueueBits;
    }
    if (slotType == kSlotTypeSpawnKeys) {
        return kSpawnKeyBits;
    }
    if (slotType == kSlotType70) {
        return kType70Bits;
    }
    return 0;
}

/** Writes one slot's auth body. */
bool write_auth_body(bits::Writer& writer,
                     const Snapshot& snapshot,
                     std::uint8_t slotType,
                     bool carriesPlayerKey) noexcept {
    const std::size_t start = writer.bit_count();
    const std::size_t expected = auth_body_bits(snapshot, slotType, carriesPlayerKey);
    bool encoded = true;
    if (slotType == kSlotType1) {
        encoded = write_type1(writer);
    } else if (slotType == kSlotType2) {
        encoded = write_type2(writer);
    } else if (slotType == kSlotType4) {
        encoded = write_type4(writer);
    } else if (slotType == kSlotType5) {
        encoded = write_type5(writer);
    } else if (slotType == kSlotTypeParticipation && carriesPlayerKey) {
        encoded = write_participation(writer, snapshot);
    } else if (slotType == kSlotTypeLifetime) {
        encoded = write_lifetime(writer, snapshot);
    } else if (slotType == kSlotTypeConfiguration) {
        // Both optional arrays absent and the terminal tag clear is the constructed state.
        encoded = writer.write(0, kPresenceWidth) && writer.write(0, kPresenceWidth)
                  && writer.write(0, kPresenceWidth) && writer.write(0, 32);
    } else if (slotType == kSlotTypePackage) {
        // 7 absent top-level fields keep the package-owned configuration.
        encoded = pad_bits(writer, kPackageBits);
    } else if (slotType == kSlotType18) {
        encoded = write_common_activity_record(writer)
                  && writer.write(0, kPresenceWidth)
                  && writer.write(kSignedZero, 32);
    } else if (slotType == kSlotType23) {
        encoded = write_type23(writer);
    } else if (slotType == kSlotType35) {
        // Both 2-bit values are biased by one, so wire value one stores logical zero.
        encoded = writer.write(0, kPresenceWidth) && writer.write(0, kPresenceWidth)
                  && writer.write(1, 2) && writer.write(1, 2)
                  && write_common_activity_record(writer);
    } else if (slotType == kSlotTypeQueues) {
        encoded = writer.write(0, 7) && writer.write(0, 5);
    } else if (slotType == kSlotTypeSpawnKeys) {
        encoded = write_spawn_keys(writer);
    } else if (slotType == kSlotType70) {
        encoded = write_type70(writer);
    }
    return encoded && writer.bit_count() == start + expected;
}

} // namespace sunrise::middleware::bap::activity_message::sensor_auth_update
