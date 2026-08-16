#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string_view>

#include "../../../core/logging/log.h"
#include "../../../state/activity/runtime.h"
#include "internal.h"
#include "task_nine_gate_observer.h"

namespace sunrise::client::hooks::bootflow {
namespace {

/**
 * The current boot-flow step accessor, `BootFlow_GetStep_NoBubbleArg`* @ `0x7FF742AED510`.
 * Decrypts the manager global and returns `mgr + 912`, or -1 when it is null. Only the call's
 * displacement is wildcarded; the `mgr + 912` field offset makes the pattern unique.
 */
constexpr std::string_view kStepSignatureText =
    "48 83 EC 28 E8 ? ? ? ? 48 85 C0 74 0B 8B 80 90 03 00 00 48 83 C4 28 C3 83 C8 FF 48 83 C4 28 "
    "C3";
/** Compiled pattern bytes of the signature text above. */
constexpr auto kStepSignature = signature<signature_length(kStepSignatureText)>(kStepSignatureText);

/** First step that loads the map with no player in it yet. */
constexpr std::int32_t kActivityLoadFirst = 33;
/** `activity:in_world`. The fade is armed by then, so a spawn now releases it. */
constexpr std::int32_t kInWorld = 38;

using GetStep = std::int64_t(__fastcall*)() noexcept;

std::atomic<GetStep> g_step{nullptr};
std::atomic<std::int32_t> g_lastStep{(std::numeric_limits<std::int32_t>::min)()};

/** Reports each boot-flow step once when it becomes current. */
void report_step(std::int32_t step) noexcept {
    const std::int32_t previous = g_lastStep.exchange(step, std::memory_order_relaxed);
    if (previous == step) {
        return;
    }
    std::array<char, core::log::kLineCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=bootflow stage=world_step result=observed step=%d",
                                      static_cast<int>(step));
    if (written > 0) {
        core::log::write(core::log::Channel::client,
                         core::log::Level::info,
                         {line.data(), static_cast<std::size_t>(written)});
    }
}

} // namespace

/** Maps the client's own boot-flow step onto the world phase. */
void observe_world_step() noexcept {
    const GetStep read = g_step.load(std::memory_order_acquire);
    if (read == nullptr) {
        return;
    }
    const auto step = static_cast<std::int32_t>(read() & 0xFFFFFFFF);
    report_step(step);
    state::activity::WorldPhase phase = state::activity::WorldPhase::idle;
    if (step == kInWorld) {
        phase = state::activity::WorldPhase::arrived;
    } else if (step >= kActivityLoadFirst && step < kInWorld) {
        phase = state::activity::WorldPhase::transitioning;
    } else {
        // Off a destination, so the next load is a fresh arming and logs its own release line.
        rearm_fade_release();
    }
    state::activity::note_world_phase(phase);
}

/** Finds the boot-flow step accessor and attaches the temporary task-9 gate observer. */
bool install_world_step() noexcept {
    std::byte* const target = scan_main_image_unique(kStepSignature, "bootflow_current_step");
    if (target == nullptr) {
        core::log::write(core::log::Channel::client,
                         core::log::Level::warn,
                         "ev=bootflow stage=world_step result=fail reason=target");
        return false;
    }
    g_step.store(reinterpret_cast<GetStep>(target), std::memory_order_release);
    if (!install_task_nine_gate_observer()) {
        g_step.store(nullptr, std::memory_order_release);
        return false;
    }
    core::log::write(core::log::Channel::client,
                     core::log::Level::info,
                     "ev=bootflow stage=world_step result=ok");
    return true;
}

/** Clears the accessor and detaches the temporary task-9 gate observer. */
void uninstall_world_step() noexcept {
    uninstall_task_nine_gate_observer();
    g_step.store(nullptr, std::memory_order_release);
    g_lastStep.store((std::numeric_limits<std::int32_t>::min)(), std::memory_order_release);
}

} // namespace sunrise::client::hooks::bootflow
