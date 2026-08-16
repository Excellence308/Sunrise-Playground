#include <algorithm>
#include <cstddef>
#include <string_view>

#include "../../../middleware/content/packages/tables/roster_intersection.h"
#include "internal.h"

namespace sunrise::client::content::scenarios {
namespace {

namespace tables = middleware::content::packages::tables;

/** @return True only for the captured supplemental group of the installed Trophy Hall scenario. */
[[nodiscard]] bool is_allowed_supplement(const Candidate& candidate,
                                         const layouts::Definition& row) noexcept {
    const std::string_view destination{row.name.data(), row.nameLength};
    return destination == kTrophyHallDestination && candidate.key == kTrophyHallAmbientKey;
}

/**
 * Orders the safe groups the way the destination publishes them.
 * A group that binds the player or reports the lifetime comes first, then one reached through the
 * destination's own registry array, then the lower key.
 * @return True when left publishes before right.
 */
[[nodiscard]] bool publishes_first(const Candidate& left, const Candidate& right) noexcept {
    const bool leftFilled = left.bindsPlayer || left.reportsLifetime;
    const bool rightFilled = right.bindsPlayer || right.reportsLifetime;
    if (leftFilled != rightFilled) {
        return leftFilled;
    }
    if (left.primaryRegistry != right.primaryRegistry) {
        return left.primaryRegistry;
    }
    return left.key < right.key;
}

} // namespace

/**
 * Keeps the candidates whose key is in every slice set and writes them into the destination row.
 * @param walk Accumulator for one destination.
 * @param row Destination row receiving its group indices.
 */
void publish_safe(Walk& walk, layouts::Definition& row) noexcept {
    row.rosterGroupCount = 0;
    row.rosterGroups = {};
    std::array<std::uint32_t, tables::kRosterKeyCapacity> safe{};
    std::size_t safeCount = 0;
    if (!tables::safe_roster_keys(walk.intersection, safe, safeCount) || safeCount == 0) {
        return;
    }
    std::array<Candidate, tables::kRosterKeyCapacity> kept{};
    std::size_t keptCount = 0;
    for (std::size_t index = 0; index < walk.candidateCount; ++index) {
        const Candidate& candidate = walk.candidates[index];
        const auto last = safe.begin() + static_cast<std::ptrdiff_t>(safeCount);
        const bool publishable = candidate.baselineRoster || is_allowed_supplement(candidate, row);
        if (publishable && std::find(safe.begin(), last, candidate.key) != last
            && keptCount < kept.size()) {
            kept[keptCount++] = candidate;
        }
    }
    std::sort(kept.begin(), kept.begin() + static_cast<std::ptrdiff_t>(keptCount), publishes_first);
    // A roster missing either filled type seeds nothing the spawn gate reads, so publish none.
    bool binds = false;
    bool reports = false;
    for (std::size_t index = 0; index < keptCount; ++index) {
        binds = binds || kept[index].bindsPlayer;
        reports = reports || kept[index].reportsLifetime;
    }
    if (!binds || !reports) {
        return;
    }
    const std::size_t published = (std::min)(keptCount, layouts::kDestinationGroupCapacity);
    for (std::size_t index = 0; index < published; ++index) {
        row.rosterGroups[index] = kept[index].group;
    }
    row.rosterGroupCount = static_cast<std::uint8_t>(published);
}

} // namespace sunrise::client::content::scenarios
