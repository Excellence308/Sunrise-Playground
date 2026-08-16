#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "../../registry.h"
#include "../../signature_text.h"

namespace sunrise::client::patterns::game::entity_spawn {

/** Top-level simulation-entity creation routine that owns the final output handle. */
inline constexpr std::string_view kCreateText =
    "40 55 53 56 57 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 "
    "48 89 85 ? ? ? ? 48 8B FA C7 02 FF FF FF FF 48 8D 54 24 ? 41 8B D9 45 8B F0 48 8B F1";

/** Allocates the 8,192-slot simulation handle used by one entity. */
inline constexpr std::string_view kAllocateHandleText =
    "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B DA C7 02 FF FF FF FF 48 8B F9 BA 00 20 "
    "00 00 48 81 C1 18 C1 00 00 E8 ? ? ? ? 4C 63 D0 41 83 FA FF";

/** Builds and commits the record behind an allocated simulation handle. */
inline constexpr std::string_view kCommitText =
    "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 55 41 56 41 57 48 83 EC ? 41 8B F9 41 8B E8 8B "
    "DA 4C 8B E9 E8 ? ? ? ? 49 8B 4D 10 8B D5 48 8B F0 E8 ? ? ? ? 4C 8B F8 41 B6 01";

/** Claims one entry from the 1,024-slot simulation-record pool. */
inline constexpr std::string_view kAllocateRecordText =
    "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 81 E2 FF 1F 00 00 48 8D 35 ? ? ? ? 33 FF 44 "
    "8B DA 48 8B D9 41 BA 00 04 00 00";

/** Applies the record's native registration mode and reports allocation failure. */
inline constexpr std::string_view kSetRegistrationModeText =
    "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F9 8B F2 32 C9 41 B8 01 00 00 00 0F B6 "
    "C9 B3 01 0F BE 47 01 83 C0 03 A9 FD FF FF FF";

inline constexpr std::size_t kCreatePatternSize = signature_length(kCreateText);
inline constexpr std::size_t kAllocateHandlePatternSize = signature_length(kAllocateHandleText);
inline constexpr std::size_t kCommitPatternSize = signature_length(kCommitText);
inline constexpr std::size_t kAllocateRecordPatternSize = signature_length(kAllocateRecordText);
inline constexpr std::size_t kSetRegistrationModePatternSize =
    signature_length(kSetRegistrationModeText);

extern constinit const std::array<patterns::PatternByte, kCreatePatternSize> kCreate;
extern constinit const std::array<patterns::PatternByte, kAllocateHandlePatternSize>
    kAllocateHandle;
extern constinit const std::array<patterns::PatternByte, kCommitPatternSize> kCommit;
extern constinit const std::array<patterns::PatternByte, kAllocateRecordPatternSize>
    kAllocateRecord;
extern constinit const std::array<patterns::PatternByte, kSetRegistrationModePatternSize>
    kSetRegistrationMode;

} // namespace sunrise::client::patterns::game::entity_spawn
