#include "entity_spawn_targets.h"

#include <array>

#include "../../../patterns/game/entity_spawn/entity_spawn_signature_bytes.h"

namespace sunrise::client::targets::game::entity_spawn {
namespace {

Targets g_targets;
bool g_resolved{};

} // namespace

/** Derives all optional static-object creation diagnostic targets in one image sweep. */
bool derive(std::span<const patterns::ImageRange> image, Targets& output) noexcept {
    output = {};
    const std::array definitions{
        patterns::Pattern{"sobject_create", patterns::game::entity_spawn::kCreate},
        patterns::Pattern{"sobject_allocate_handle", patterns::game::entity_spawn::kAllocateHandle},
        patterns::Pattern{"sobject_commit", patterns::game::entity_spawn::kCommit},
        patterns::Pattern{"sobject_allocate_record", patterns::game::entity_spawn::kAllocateRecord},
        patterns::Pattern{"sobject_set_registration_mode",
                          patterns::game::entity_spawn::kSetRegistrationMode},
    };
    std::array<patterns::Match, definitions.size()> matches{};
    if (!patterns::resolve_all(image, definitions, matches)) {
        return false;
    }
    for (const patterns::Match match : matches) {
        if (match.status != patterns::MatchStatus::unique) {
            return false;
        }
    }

    output = Targets{matches[0].address,
                     matches[1].address,
                     matches[2].address,
                     matches[3].address,
                     matches[4].address};
    return true;
}

/** Publishes a completely resolved optional target table. */
void publish(const Targets& targets) noexcept {
    g_targets = targets;
    g_resolved = true;
}

/** Clears every published entity-spawn entry. */
void clear() noexcept {
    g_targets = {};
    g_resolved = false;
}

/** @return Process-local entity-spawn target group. */
const Targets& get() noexcept {
    return g_targets;
}

/** @return True after all five entry points are published. */
bool is_resolved() noexcept {
    return g_resolved;
}

} // namespace sunrise::client::targets::game::entity_spawn
