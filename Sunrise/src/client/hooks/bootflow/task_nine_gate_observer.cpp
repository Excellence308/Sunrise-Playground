#include "task_nine_gate_observer.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <intrin.h>
#include <string_view>

#include "../../../core/logging/log.h"
#include "../../hooking/detour.h"
#include "internal.h"

namespace sunrise::client::hooks::bootflow {
namespace {

/** Initial-slice task 9 in the installed Shadowkeep client. */
constexpr std::string_view kTaskNineSignatureText =
    "40 53 57 48 83 EC 48 48 8B D9 BF 01 00 00 00 E8 ? ? ? ? 84 C0 75 0D E8 ? ? ? ? 48 8B C8 "
    "E8 ? ? ? ? 80 3D ? ? ? ? 00 74 1A";
constexpr auto kTaskNineSignature =
    signature<signature_length(kTaskNineSignatureText)>(kTaskNineSignatureText);

/** Stable context offsets of the four task-state masks. */
constexpr std::array<std::size_t, 4> kTaskMaskOffsets{0x198, 0x1A0, 0x1A8, 0x1B0};
/** Tasks whose states task 9 reads directly. */
constexpr std::array<std::uint32_t, 5> kObservedTaskIds{0, 2, 5, 6, 8};
/** Original task-9 bytes retained for offline analysis, including the final predicate call. */
constexpr std::size_t kTaskNineCaptureSize = 0x290;
/** Bounded capture of each helper reached directly from task 9. */
constexpr std::size_t kHelperCaptureSize = 0x400;
/** Keeps each hexadecimal capture record well below the logging line limit. */
constexpr std::size_t kCodeCaptureChunk = 0x100;

/** Task-9-relative instruction locations verified against the signature and saved image. */
constexpr std::size_t kEarlyGateInstruction = 0x25;
constexpr std::size_t kObjectIdCall = 0x5A;
constexpr std::size_t kSelectorCall = 0x64;
constexpr std::size_t kSelectedObjectCall = 0x86;
constexpr std::size_t kFinalPredicateCall = 0x22F;
/** Nested calls verified in the captured selector and selected-object helpers. */
constexpr std::size_t kSelectorSourceCall = 0x09;
constexpr std::size_t kObjectResolverCall = 0x68;
/** Return-address offsets distinguish the three direct calls from nested calls on the same thread.
 */
constexpr std::size_t kSelectorReturn = kSelectorCall + 5;
constexpr std::size_t kSelectedObjectReturn = kSelectedObjectCall + 5;
constexpr std::size_t kFinalPredicateReturn = kFinalPredicateCall + 5;
/** `call rel32` and `cmp byte ptr [rip+rel32], imm8` encodings used above. */
constexpr std::size_t kCallOperand = 1;
constexpr std::size_t kCallLength = 5;
constexpr std::size_t kEarlyGateOperand = 2;
constexpr std::size_t kEarlyGateInstructionLength = 7;

enum HookSlot : std::size_t {
    taskNineSlot,
    selectorSlot,
    selectorSourceSlot,
    selectedObjectSlot,
    objectResolverSlot,
    finalPredicateSlot,
    hookCount,
};

using TaskNine = std::int32_t(__fastcall*)(std::byte*) noexcept;
using Selector = bool(__fastcall*)(std::int32_t*) noexcept;
using SelectorSource = std::byte*(__fastcall*)() noexcept;
using SelectedObject = std::byte*(__fastcall*)(std::uint32_t) noexcept;
using ObjectResolver = std::byte*(__fastcall*)(std::byte*) noexcept;
using FinalPredicate = bool(__fastcall*)() noexcept;

struct HelperSample {
    bool selectorCalled{};
    bool selectorReadable{};
    bool selectorResult{};
    std::int32_t selectorValue{};
    bool selectorSourceCalled{};
    bool selectorSourcePresent{};
    bool selectorSourceReadable{};
    std::uint8_t selectorSourceByte{};
    bool objectCalled{};
    bool objectPresent{};
    bool objectReadable{};
    std::uint32_t objectId{};
    std::uint8_t objectByte{};
    bool objectResolverCalled{};
    bool objectResolverPresent{};
    bool finalCalled{};
    bool finalResult{};
};

struct Snapshot {
    std::int32_t result{};
    std::int32_t nativeResult{};
    bool resultForced{};
    bool earlyReadable{};
    std::uint8_t earlyValue{};
    std::array<std::int32_t, kObservedTaskIds.size()> taskStates{};
    HelperSample helpers{};
};

std::array<hooking::detour::Handle, hookCount> g_hooks{};
std::array<std::atomic<void*>, hookCount> g_originals{};
std::atomic_bool g_ready{false};
std::atomic<std::byte*> g_taskNineTarget{nullptr};
std::atomic<const std::byte*> g_earlyGate{nullptr};

thread_local bool g_inTaskNine{};
thread_local bool g_inSelector{};
thread_local bool g_inSelectedObject{};
thread_local HelperSample g_helperSample{};
thread_local bool g_hasLastSnapshot{};
thread_local Snapshot g_lastSnapshot{};

/** Waits only across the tiny post-transaction publication window. */
template <typename Function> [[nodiscard]] Function original(HookSlot slot) noexcept {
    while (!g_ready.load(std::memory_order_acquire)) {
        _mm_pause();
    }
    return reinterpret_cast<Function>(
        g_originals[static_cast<std::size_t>(slot)].load(std::memory_order_acquire));
}

/** Reads one unaligned task mask from the valid task callback context. */
[[nodiscard]] std::uint64_t task_mask(const std::byte* context, std::size_t offset) noexcept {
    std::uint64_t value{};
    std::memcpy(&value, context + offset, sizeof(value));
    return value;
}

/** Maps the four native masks onto the state value task 9 itself computes. */
[[nodiscard]] std::int32_t task_state(const std::byte* context, std::uint32_t taskId) noexcept {
    const std::uint64_t bit = std::uint64_t{1} << taskId;
    for (std::size_t state = kTaskMaskOffsets.size(); state > 0; --state) {
        if ((task_mask(context, kTaskMaskOffsets[state - 1]) & bit) != 0) {
            return static_cast<std::int32_t>(state - 1);
        }
    }
    return -1;
}

/** True only for a direct helper call from the observed task-9 body. */
[[nodiscard]] bool is_task_nine_return(const void* caller, std::size_t offset) noexcept {
    const std::byte* const target = g_taskNineTarget.load(std::memory_order_acquire);
    return target != nullptr && caller == target + offset;
}

/** Reads one byte defensively because the selected object is owned by the Client. */
[[nodiscard]] bool read_byte(const std::byte* address, std::uint8_t& value) noexcept {
    if (address == nullptr) {
        return false;
    }
    __try {
        value = static_cast<std::uint8_t>(*address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

/** Reads the selector output defensively after the native helper filled it. */
[[nodiscard]] bool read_selector(const std::int32_t* address, std::int32_t& value) noexcept {
    if (address == nullptr) {
        return false;
    }
    __try {
        value = *address;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

/** Exact field comparison avoids padding-dependent diagnostic churn. */
[[nodiscard]] bool same_sample(const HelperSample& left, const HelperSample& right) noexcept {
    return left.selectorCalled == right.selectorCalled
           && left.selectorReadable == right.selectorReadable
           && left.selectorResult == right.selectorResult
           && left.selectorValue == right.selectorValue
           && left.selectorSourceCalled == right.selectorSourceCalled
           && left.selectorSourcePresent == right.selectorSourcePresent
           && left.selectorSourceReadable == right.selectorSourceReadable
           && left.selectorSourceByte == right.selectorSourceByte
           && left.objectCalled == right.objectCalled && left.objectPresent == right.objectPresent
           && left.objectReadable == right.objectReadable && left.objectId == right.objectId
           && left.objectByte == right.objectByte
           && left.objectResolverCalled == right.objectResolverCalled
           && left.objectResolverPresent == right.objectResolverPresent
           && left.finalCalled == right.finalCalled && left.finalResult == right.finalResult;
}

[[nodiscard]] bool same_snapshot(const Snapshot& left, const Snapshot& right) noexcept {
    return left.result == right.result && left.nativeResult == right.nativeResult
           && left.resultForced == right.resultForced && left.earlyReadable == right.earlyReadable
           && left.earlyValue == right.earlyValue && left.taskStates == right.taskStates
           && same_sample(left.helpers, right.helpers);
}

/** Records unpacked runtime code in bounded hexadecimal chunks before detouring it. */
void report_code(std::string_view stage,
                 const std::byte* target,
                 std::size_t captureSize) noexcept {
    constexpr char hex[] = "0123456789ABCDEF";
    for (std::size_t offset = 0; offset < captureSize; offset += kCodeCaptureChunk) {
        const std::size_t count = (std::min)(kCodeCaptureChunk, captureSize - offset);
        std::array<char, core::log::kLineCapacity> line{};
        const int prefix = std::snprintf(line.data(),
                                         line.size(),
                                         "ev=bootflow stage=%.*s result=observed "
                                         "offset=0x%03zX bytes=",
                                         static_cast<int>(stage.size()),
                                         stage.data(),
                                         offset);
        if (prefix <= 0) {
            return;
        }
        std::size_t cursor = static_cast<std::size_t>(prefix);
        for (std::size_t index = 0; index < count; ++index) {
            std::uint8_t value{};
            if (!read_byte(target + offset + index, value) || cursor + 2 >= line.size()) {
                return;
            }
            line[cursor++] = hex[value >> 4U];
            line[cursor++] = hex[value & 0x0FU];
        }
        core::log::write(core::log::Channel::client, core::log::Level::info, {line.data(), cursor});
    }
}

/** Logs only gate-state transitions, keeping a permanently pending task bounded. */
void report_task_nine(const std::byte* context,
                      std::int32_t result,
                      std::int32_t nativeResult,
                      bool resultForced,
                      bool earlyReadable,
                      std::uint8_t earlyValue,
                      const HelperSample& helpers) noexcept {
    if (context == nullptr) {
        return;
    }
    Snapshot snapshot{};
    snapshot.result = result;
    snapshot.nativeResult = nativeResult;
    snapshot.resultForced = resultForced;
    snapshot.earlyReadable = earlyReadable;
    snapshot.earlyValue = earlyValue;
    snapshot.helpers = helpers;
    for (std::size_t index = 0; index < kObservedTaskIds.size(); ++index) {
        snapshot.taskStates[index] = task_state(context, kObservedTaskIds[index]);
    }
    if (g_hasLastSnapshot && same_snapshot(snapshot, g_lastSnapshot)) {
        return;
    }
    g_hasLastSnapshot = true;
    g_lastSnapshot = snapshot;

    std::array<char, core::log::kLineCapacity> line{};
    const int written =
        std::snprintf(line.data(),
                      line.size(),
                      "ev=bootflow stage=task_9_gates result=observed return=%d native_return=%d "
                      "return_forced=%u early=%u,%u "
                      "states_0_2_5_6_8=%d,%d,%d,%d,%d selector=%u,%u,%u,%d "
                      "selector_source=%u,%u,%u,%u object=%u,%u,%u,0x%08X,%u "
                      "object_resolver=%u,%u final=%u,%u",
                      static_cast<int>(snapshot.result),
                      static_cast<int>(snapshot.nativeResult),
                      snapshot.resultForced ? 1U : 0U,
                      snapshot.earlyReadable ? 1U : 0U,
                      static_cast<unsigned>(snapshot.earlyValue),
                      static_cast<int>(snapshot.taskStates[0]),
                      static_cast<int>(snapshot.taskStates[1]),
                      static_cast<int>(snapshot.taskStates[2]),
                      static_cast<int>(snapshot.taskStates[3]),
                      static_cast<int>(snapshot.taskStates[4]),
                      snapshot.helpers.selectorCalled ? 1U : 0U,
                      snapshot.helpers.selectorReadable ? 1U : 0U,
                      snapshot.helpers.selectorResult ? 1U : 0U,
                      static_cast<int>(snapshot.helpers.selectorValue),
                      snapshot.helpers.selectorSourceCalled ? 1U : 0U,
                      snapshot.helpers.selectorSourcePresent ? 1U : 0U,
                      snapshot.helpers.selectorSourceReadable ? 1U : 0U,
                      static_cast<unsigned>(snapshot.helpers.selectorSourceByte),
                      snapshot.helpers.objectCalled ? 1U : 0U,
                      snapshot.helpers.objectPresent ? 1U : 0U,
                      snapshot.helpers.objectReadable ? 1U : 0U,
                      static_cast<unsigned>(snapshot.helpers.objectId),
                      static_cast<unsigned>(snapshot.helpers.objectByte),
                      snapshot.helpers.objectResolverCalled ? 1U : 0U,
                      snapshot.helpers.objectResolverPresent ? 1U : 0U,
                      snapshot.helpers.finalCalled ? 1U : 0U,
                      snapshot.helpers.finalResult ? 1U : 0U);
    if (written > 0) {
        core::log::write(core::log::Channel::client,
                         core::log::Level::info,
                         {line.data(), static_cast<std::size_t>(written)});
    }
}

/** Pass-through selector source; distinguishes a null source from its `0xFF` sentinel. */
__declspec(noinline) std::byte* __fastcall selector_source() noexcept {
    std::byte* const result = original<SelectorSource>(selectorSourceSlot)();
    if (g_inTaskNine && g_inSelector) {
        g_helperSample.selectorSourceCalled = true;
        g_helperSample.selectorSourcePresent = result != nullptr;
        g_helperSample.selectorSourceReadable =
            read_byte(result, g_helperSample.selectorSourceByte);
    }
    return result;
}

/** Pass-through selector helper; records only the direct call made by task 9. */
__declspec(noinline) bool __fastcall selector(std::int32_t* value) noexcept {
    const void* const caller = _ReturnAddress();
    g_inSelector = true;
    const bool result = original<Selector>(selectorSlot)(value);
    g_inSelector = false;
    if (g_inTaskNine && is_task_nine_return(caller, kSelectorReturn)) {
        g_helperSample.selectorCalled = true;
        g_helperSample.selectorResult = result;
        g_helperSample.selectorReadable =
            result && read_selector(value, g_helperSample.selectorValue);
    }
    return result;
}

/** Pass-through selected-object lookup; records only the direct call made by task 9. */
__declspec(noinline) std::byte* __fastcall selected_object(std::uint32_t id) noexcept {
    const void* const caller = _ReturnAddress();
    g_inSelectedObject = true;
    std::byte* const result = original<SelectedObject>(selectedObjectSlot)(id);
    g_inSelectedObject = false;
    if (g_inTaskNine && is_task_nine_return(caller, kSelectedObjectReturn)) {
        g_helperSample.objectCalled = true;
        g_helperSample.objectPresent = result != nullptr;
        g_helperSample.objectId = id;
        g_helperSample.objectReadable =
            read_byte(result != nullptr ? result + 8 : nullptr, g_helperSample.objectByte);
    }
    return result;
}

/** Pass-through linked-object resolver; records whether the primary row reaches it. */
__declspec(noinline) std::byte* __fastcall object_resolver(std::byte* row) noexcept {
    std::byte* const result = original<ObjectResolver>(objectResolverSlot)(row);
    if (g_inTaskNine && g_inSelectedObject) {
        g_helperSample.objectResolverCalled = true;
        g_helperSample.objectResolverPresent = result != nullptr;
    }
    return result;
}

/** Pass-through final predicate; records only the direct call made by task 9. */
__declspec(noinline) bool __fastcall final_predicate() noexcept {
    const void* const caller = _ReturnAddress();
    const bool result = original<FinalPredicate>(finalPredicateSlot)();
    if (g_inTaskNine && is_task_nine_return(caller, kFinalPredicateReturn)) {
        g_helperSample.finalCalled = true;
        g_helperSample.finalResult = result;
    }
    return result;
}

/** Runs task 9 natively and reports its direct helper results unchanged. */
__declspec(noinline) std::int32_t __fastcall task_nine(std::byte* context) noexcept {
    if (g_inTaskNine) {
        return original<TaskNine>(taskNineSlot)(context);
    }
    g_helperSample = {};
    std::uint8_t earlyValue{};
    const bool earlyReadable = read_byte(g_earlyGate.load(std::memory_order_acquire), earlyValue);
    g_inTaskNine = true;
    const std::int32_t nativeResult = original<TaskNine>(taskNineSlot)(context);
    g_inTaskNine = false;
    report_task_nine(
        context, nativeResult, nativeResult, false, earlyReadable, earlyValue, g_helperSample);
    return nativeResult;
}

/** Confirms every fixed instruction used to resolve a helper from the task-9 body. */
[[nodiscard]] bool valid_layout(const std::byte* target) noexcept {
    const auto byte = [target](std::size_t offset) {
        return std::to_integer<std::uint8_t>(target[offset]);
    };
    return byte(kEarlyGateInstruction) == 0x80 && byte(kEarlyGateInstruction + 1) == 0x3D
           && byte(kEarlyGateInstruction + 6) == 0x00 && byte(kObjectIdCall) == 0xE8
           && byte(kSelectorCall) == 0xE8 && byte(kSelectedObjectCall) == 0xE8
           && byte(kFinalPredicateCall) == 0xE8;
}

/** Confirms the nested call instructions used by the second-level observer. */
[[nodiscard]] bool valid_helper_layout(const std::byte* selectorTarget,
                                       const std::byte* selectedObjectTarget) noexcept {
    return std::to_integer<std::uint8_t>(selectorTarget[kSelectorSourceCall]) == 0xE8
           && std::to_integer<std::uint8_t>(selectedObjectTarget[kObjectResolverCall]) == 0xE8;
}

/** Reports one installation failure without leaving a partial detour batch. */
[[nodiscard]] bool fail(const char* reason) noexcept {
    std::array<char, 160> line{};
    const int written = std::snprintf(
        line.data(), line.size(), "ev=bootflow stage=task_9_gates result=fail reason=%s", reason);
    if (written > 0) {
        core::log::write(core::log::Channel::client,
                         core::log::Level::warn,
                         {line.data(), static_cast<std::size_t>(written)});
    }
    return false;
}

} // namespace

/** Attaches the pass-through initial-slice task-9 and helper-code diagnostic. */
bool install_task_nine_gate_observer() noexcept {
    std::byte* const target = scan_main_image_unique(kTaskNineSignature, "initial_slice_task_9");
    if (target == nullptr) {
        return fail("target");
    }
    if (!valid_layout(target)) {
        return fail("layout");
    }
    std::byte* const earlyGate =
        resolve_relative(target + kEarlyGateInstruction + kEarlyGateOperand,
                         target + kEarlyGateInstruction + kEarlyGateInstructionLength);
    std::byte* const objectIdTarget = resolve_relative(target + kObjectIdCall + kCallOperand,
                                                       target + kObjectIdCall + kCallLength);
    std::byte* const selectorTarget = resolve_relative(target + kSelectorCall + kCallOperand,
                                                       target + kSelectorCall + kCallLength);
    std::byte* const selectedObjectTarget = resolve_relative(
        target + kSelectedObjectCall + kCallOperand, target + kSelectedObjectCall + kCallLength);
    std::byte* const finalPredicateTarget = resolve_relative(
        target + kFinalPredicateCall + kCallOperand, target + kFinalPredicateCall + kCallLength);
    if (earlyGate == nullptr || objectIdTarget == nullptr || selectorTarget == nullptr
        || selectedObjectTarget == nullptr || finalPredicateTarget == nullptr) {
        return fail("resolve");
    }
    if (!valid_helper_layout(selectorTarget, selectedObjectTarget)) {
        return fail("helper_layout");
    }
    std::byte* const selectorSourceTarget =
        resolve_relative(selectorTarget + kSelectorSourceCall + kCallOperand,
                         selectorTarget + kSelectorSourceCall + kCallLength);
    std::byte* const objectResolverTarget =
        resolve_relative(selectedObjectTarget + kObjectResolverCall + kCallOperand,
                         selectedObjectTarget + kObjectResolverCall + kCallLength);
    if (selectorSourceTarget == nullptr || objectResolverTarget == nullptr) {
        return fail("helper_resolve");
    }

    report_code("task_9_code", target, kTaskNineCaptureSize);
    report_code("task_9_object_id_code", objectIdTarget, kHelperCaptureSize);
    report_code("task_9_selector_code", selectorTarget, kHelperCaptureSize);
    report_code("task_9_selector_source_code", selectorSourceTarget, kHelperCaptureSize);
    report_code("task_9_object_lookup_code", selectedObjectTarget, kHelperCaptureSize);
    report_code("task_9_object_resolver_code", objectResolverTarget, kHelperCaptureSize);

    g_taskNineTarget.store(target, std::memory_order_release);
    g_earlyGate.store(earlyGate, std::memory_order_release);
    const std::array<hooking::detour::Spec, hookCount> specs{
        hooking::detour::Spec{target, reinterpret_cast<void*>(&task_nine)},
        hooking::detour::Spec{selectorTarget, reinterpret_cast<void*>(&selector)},
        hooking::detour::Spec{selectorSourceTarget, reinterpret_cast<void*>(&selector_source)},
        hooking::detour::Spec{selectedObjectTarget, reinterpret_cast<void*>(&selected_object)},
        hooking::detour::Spec{objectResolverTarget, reinterpret_cast<void*>(&object_resolver)},
        hooking::detour::Spec{finalPredicateTarget, reinterpret_cast<void*>(&final_predicate)},
    };
    if (!hooking::detour::install(specs, g_hooks)) {
        g_taskNineTarget.store(nullptr, std::memory_order_release);
        g_earlyGate.store(nullptr, std::memory_order_release);
        return fail("attach");
    }
    for (std::size_t index = 0; index < g_hooks.size(); ++index) {
        g_originals[index].store(g_hooks[index].original, std::memory_order_release);
    }
    g_ready.store(true, std::memory_order_release);
    core::log::write(core::log::Channel::client,
                     core::log::Level::info,
                     "ev=bootflow stage=task_9_gates result=ok mode=pass_through_nested_observer");
    return true;
}

/** Detaches the pass-through initial-slice task-9 and helper-code diagnostic. */
void uninstall_task_nine_gate_observer() noexcept {
    if (!g_hooks[taskNineSlot].attached || !hooking::detour::uninstall(g_hooks)) {
        return;
    }
    g_ready.store(false, std::memory_order_release);
    for (auto& originalEntry : g_originals) {
        originalEntry.store(nullptr, std::memory_order_release);
    }
    g_taskNineTarget.store(nullptr, std::memory_order_release);
    g_earlyGate.store(nullptr, std::memory_order_release);
}

} // namespace sunrise::client::hooks::bootflow
