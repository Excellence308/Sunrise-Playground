#include "entity_spawn_signature_bytes.h"

namespace sunrise::client::patterns::game::entity_spawn {

constinit const std::array<patterns::PatternByte, kCreatePatternSize> kCreate =
    signature<kCreatePatternSize>(kCreateText);
constinit const std::array<patterns::PatternByte, kAllocateHandlePatternSize> kAllocateHandle =
    signature<kAllocateHandlePatternSize>(kAllocateHandleText);
constinit const std::array<patterns::PatternByte, kCommitPatternSize> kCommit =
    signature<kCommitPatternSize>(kCommitText);
constinit const std::array<patterns::PatternByte, kAllocateRecordPatternSize> kAllocateRecord =
    signature<kAllocateRecordPatternSize>(kAllocateRecordText);
constinit const std::array<patterns::PatternByte, kSetRegistrationModePatternSize>
    kSetRegistrationMode = signature<kSetRegistrationModePatternSize>(kSetRegistrationModeText);

} // namespace sunrise::client::patterns::game::entity_spawn
