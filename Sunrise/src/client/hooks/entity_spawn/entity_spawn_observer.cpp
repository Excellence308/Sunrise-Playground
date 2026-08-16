#include "entity_spawn_observer.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string_view>

#include "../../../core/logging/log.h"

namespace sunrise::client::hooks::entity_spawn {
namespace {

using Create = std::int32_t*(__fastcall*)(void*,
                                          std::int32_t*,
                                          std::int32_t,
                                          std::int32_t,
                                          std::int32_t) noexcept;
using AllocateHandle = std::int32_t*(__fastcall*)(void*, std::int32_t*) noexcept;
using Commit = bool(__fastcall*)(void*,
                                 std::int32_t,
                                 std::int32_t,
                                 std::int32_t,
                                 std::int32_t) noexcept;
using AllocateRecord = void*(__fastcall*)(void*, std::int32_t) noexcept;
using SetRegistrationMode = bool(__fastcall*)(void*, std::int32_t) noexcept;

/** Initial Hall load emits about 138 sobject failures; retain two passes without unbounded logs. */
constexpr std::uint32_t kFailureLimit = 256;
constexpr std::size_t kLineCapacity = 384;
constexpr std::size_t kHandleSlotCount = 0x2000;
constexpr std::size_t kHandleSlotStride = 6;
constexpr std::size_t kMappingOffset = 0x114;
constexpr std::size_t kGenerationOffset = 0x118;
constexpr std::size_t kFreeBitmapOffset = 0xC118;
constexpr std::size_t kFreeBitmapWordCount = kHandleSlotCount / 32;

struct TraceContext {
    std::uint32_t depth{};
    std::int32_t sourceHandle{-1};
    std::int32_t createArgument{-1};
    std::int32_t createMode{-1};
    std::int32_t simulationHandle{-1};
    std::uintptr_t handleManager{};
    bool allocateHandleCalled{};
    bool allocateHandleSucceeded{};
    bool commitCalled{};
    bool commitSucceeded{};
    bool allocateRecordCalled{};
    bool allocateRecordSucceeded{};
    bool registrationCalled{};
    bool registrationSucceeded{};
};

struct PendingFailure {
    bool available{};
    TraceContext trace{};
};

struct HandlePoolState {
    bool poolReadable{};
    bool ownerReadable{};
    bool ownerPresent{};
    bool failureSinkPresent{};
    std::uint32_t freeBits{};
    std::uint32_t nonzeroFreeWords{};
    std::uint32_t generationsNonzero{};
    std::uint32_t mappingsMinusOne{};
    std::uint32_t mappingsOther{};
    std::uint32_t ownerEpoch{};
};

thread_local TraceContext g_trace;
thread_local PendingFailure g_pendingFailure;
std::atomic_uint32_t g_failureCount{};
std::atomic_bool g_correlationMissReported{};
std::atomic_bool g_handlePoolStateReported{};

/** @param slot Stable diagnostic slot. @return Its installed trampoline, or null. */
template <typename Function>
[[nodiscard]] Function original(Slot slot) noexcept {
    return reinterpret_cast<Function>(g_handles[static_cast<std::size_t>(slot)].original);
}

/** Reads the output owned by the original routine without trusting a borrowed pointer. */
[[nodiscard]] std::int32_t read_output(const std::int32_t* value) noexcept {
    if (value == nullptr) {
        return -1;
    }
    __try {
        return *value;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

[[nodiscard]] std::uint32_t count_set_bits(std::uint32_t value) noexcept {
    std::uint32_t count = 0;
    while (value != 0) {
        value &= value - 1;
        ++count;
    }
    return count;
}

/** Reads the allocator fields proven by the archived native helper, without changing them. */
[[nodiscard]] HandlePoolState inspect_handle_pool(std::uintptr_t managerAddress) noexcept {
    HandlePoolState state{};
    if (managerAddress == 0) {
        return state;
    }
    const auto* const base =
        reinterpret_cast<const volatile std::uint8_t*>(managerAddress);
    __try {
        for (std::size_t word = 0; word < kFreeBitmapWordCount; ++word) {
            const auto* const entry = reinterpret_cast<const volatile std::uint32_t*>(
                base + kFreeBitmapOffset + word * sizeof(std::uint32_t));
            const std::uint32_t value = *entry;
            state.freeBits += count_set_bits(value);
            state.nonzeroFreeWords += value != 0 ? 1U : 0U;
        }
        for (std::size_t slot = 0; slot < kHandleSlotCount; ++slot) {
            const auto* const mapping = reinterpret_cast<const volatile std::int16_t*>(
                base + kMappingOffset + slot * kHandleSlotStride);
            const auto* const generation = reinterpret_cast<const volatile std::uint8_t*>(
                base + kGenerationOffset + slot * kHandleSlotStride);
            state.generationsNonzero += *generation != 0 ? 1U : 0U;
            if (*mapping == -1) {
                ++state.mappingsMinusOne;
            } else {
                ++state.mappingsOther;
            }
        }
        state.poolReadable = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        state.poolReadable = false;
    }
    __try {
        const auto* const ownerSlot = reinterpret_cast<const volatile std::uintptr_t*>(
            base + sizeof(std::uintptr_t));
        const std::uintptr_t ownerAddress = *ownerSlot;
        state.ownerPresent = ownerAddress != 0;
        if (state.ownerPresent) {
            state.ownerEpoch = *reinterpret_cast<const volatile std::uint32_t*>(
                ownerAddress + sizeof(std::uintptr_t));
            state.failureSinkPresent =
                *reinterpret_cast<const volatile std::uintptr_t*>(
                    ownerAddress + 2 * sizeof(std::uintptr_t))
                != 0;
        }
        state.ownerReadable = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        state.ownerReadable = false;
    }
    return state;
}

/** Emits one bounded allocator-state summary for the first exact sobject handle failure. */
void report_handle_pool_state(const TraceContext& trace) noexcept {
    if (trace.handleManager == 0
        || g_handlePoolStateReported.exchange(true, std::memory_order_relaxed)) {
        return;
    }
    const HandlePoolState state = inspect_handle_pool(trace.handleManager);
    std::array<char, kLineCapacity> line{};
    const int written = std::snprintf(
        line.data(),
        line.size(),
        "ev=sobject_trace stage=handle_pool_state result=%s manager=0x%zX "
        "free_bits=%u free_words=%u generation_nonzero=%u "
        "mapping_minus_one=%u mapping_other=%u owner_readable=%u owner=%u "
        "owner_epoch=0x%08X failure_sink=%u",
        state.poolReadable ? "ok" : "unreadable",
        static_cast<std::size_t>(trace.handleManager),
        state.freeBits,
        state.nonzeroFreeWords,
        state.generationsNonzero,
        state.mappingsMinusOne,
        state.mappingsOther,
        state.ownerReadable ? 1U : 0U,
        state.ownerPresent ? 1U : 0U,
        state.ownerEpoch,
        state.failureSinkPresent ? 1U : 0U);
    if (written <= 0) {
        return;
    }
    const auto length = static_cast<std::size_t>(written) < line.size()
                            ? static_cast<std::size_t>(written)
                            : line.size() - 1;
    core::log::write(core::log::Channel::client,
                     core::log::Level::info,
                     {line.data(), length});
}

/** @return The narrowest native stage proven by the nested observer results. */
[[nodiscard]] std::string_view failure_reason(const TraceContext& trace) noexcept {
    if (!trace.allocateHandleCalled || !trace.allocateHandleSucceeded) {
        return "handle_pool";
    }
    if (!trace.commitCalled) {
        return "commit_not_reached";
    }
    if (!trace.allocateRecordCalled || !trace.allocateRecordSucceeded) {
        return "record_pool";
    }
    if (!trace.registrationCalled) {
        return "buffer_setup";
    }
    if (!trace.registrationSucceeded) {
        return "registration";
    }
    if (!trace.commitSucceeded) {
        return "commit_unknown";
    }
    return "create_unknown";
}

/** Emits one bounded failure event after the native retail line identifies its entity type. */
void report_failure(const TraceContext& trace, std::string_view entityName) noexcept {
    if (g_failureCount.fetch_add(1, std::memory_order_relaxed) >= kFailureLimit) {
        return;
    }
    const std::string_view reason = failure_reason(trace);
    std::array<char, kLineCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=sobject_trace stage=create entity=%.*s result=fail "
                                      "reason=%.*s "
                                      "manager=0x%zX source=0x%08X create_arg=%d create_mode=%d "
                                      "sim=0x%08X "
                                      "commit=%u record=%u registration=%u",
                                      static_cast<int>(entityName.size()),
                                      entityName.data(),
                                      static_cast<int>(reason.size()),
                                      reason.data(),
                                      static_cast<std::size_t>(trace.handleManager),
                                      static_cast<std::uint32_t>(trace.sourceHandle),
                                      trace.createArgument,
                                      trace.createMode,
                                      static_cast<std::uint32_t>(trace.simulationHandle),
                                      trace.commitCalled ? 1U : 0U,
                                      trace.allocateRecordSucceeded ? 1U : 0U,
                                      trace.registrationSucceeded ? 1U : 0U);
    if (written <= 0) {
        return;
    }
    const auto length = static_cast<std::size_t>(written) < line.size()
                            ? static_cast<std::size_t>(written)
                            : line.size() - 1;
    core::log::write(core::log::Channel::client,
                     core::log::Level::info,
                     {line.data(), length});
}

/** Observes one complete entity-create call and correlates its nested stages by thread. */
__declspec(noinline) std::int32_t* __fastcall create_body(void* manager,
                                                          std::int32_t* output,
                                                          std::int32_t createArgument,
                                                          std::int32_t sourceHandle,
                                                          std::int32_t createMode) noexcept {
    const bool outer = g_trace.depth++ == 0;
    if (outer) {
        g_trace = TraceContext{1, sourceHandle, createArgument, createMode};
        g_pendingFailure = {};
    }
    const auto call = original<Create>(Slot::create);
    std::int32_t* const result =
        call != nullptr ? call(manager, output, createArgument, sourceHandle, createMode) : output;
    if (outer) {
        if (read_output(result != nullptr ? result : output) == -1) {
            g_pendingFailure = PendingFailure{true, g_trace};
        }
        g_trace = {};
    } else {
        --g_trace.depth;
    }
    return result;
}

/** Records whether the native 8,192-slot simulation handle allocator succeeded. */
__declspec(noinline) std::int32_t* __fastcall allocate_handle_body(void* manager,
                                                                   std::int32_t* output) noexcept {
    const auto call = original<AllocateHandle>(Slot::allocateHandle);
    std::int32_t* const result = call != nullptr ? call(manager, output) : output;
    if (g_trace.depth == 1) {
        g_trace.handleManager = reinterpret_cast<std::uintptr_t>(manager);
        g_trace.allocateHandleCalled = true;
        g_trace.simulationHandle = read_output(result != nullptr ? result : output);
        g_trace.allocateHandleSucceeded = g_trace.simulationHandle != -1;
    }
    return result;
}

/** Records the aggregate native record-build and commit result. */
__declspec(noinline) bool __fastcall commit_body(void* manager,
                                                 std::int32_t simulationHandle,
                                                 std::int32_t createArgument,
                                                 std::int32_t sourceHandle,
                                                 std::int32_t createMode) noexcept {
    if (g_trace.depth == 1) {
        g_trace.commitCalled = true;
        g_trace.simulationHandle = simulationHandle;
        g_trace.sourceHandle = sourceHandle;
        g_trace.createArgument = createArgument;
        g_trace.createMode = createMode;
    }
    const auto call = original<Commit>(Slot::commit);
    const bool result = call != nullptr
                            && call(manager,
                                    simulationHandle,
                                    createArgument,
                                    sourceHandle,
                                    createMode);
    if (g_trace.depth == 1) {
        g_trace.commitSucceeded = result;
    }
    return result;
}

/** Records whether the native 1,024-slot entity-record allocator produced a record. */
__declspec(noinline) void* __fastcall allocate_record_body(void* manager,
                                                            std::int32_t simulationHandle) noexcept {
    const auto call = original<AllocateRecord>(Slot::allocateRecord);
    void* const result = call != nullptr ? call(manager, simulationHandle) : nullptr;
    if (g_trace.depth == 1) {
        g_trace.allocateRecordCalled = true;
        g_trace.allocateRecordSucceeded = result != nullptr;
    }
    return result;
}

/** Records whether the final native registration-mode transition succeeded. */
__declspec(noinline) bool __fastcall set_registration_mode_body(void* record,
                                                                std::int32_t mode) noexcept {
    const auto call = original<SetRegistrationMode>(Slot::setRegistrationMode);
    const bool result = call != nullptr && call(record, mode);
    if (g_trace.depth == 1) {
        g_trace.registrationCalled = true;
        g_trace.registrationSucceeded = result;
    }
    return result;
}

} // namespace

void report_pending_failure(std::string_view entityName) noexcept {
    if (!g_pendingFailure.available) {
        if (!g_correlationMissReported.exchange(true, std::memory_order_relaxed)) {
            core::log::write(core::log::Channel::client,
                             core::log::Level::warn,
                             "ev=sobject_trace stage=correlate result=miss");
        }
        return;
    }
    const TraceContext trace = g_pendingFailure.trace;
    g_pendingFailure = {};
    report_failure(trace, entityName);
    if (!trace.allocateHandleSucceeded) {
        report_handle_pool_state(trace);
    }
}

void* create_entry_point() noexcept {
    return reinterpret_cast<void*>(&create_body);
}

void* allocate_handle_entry_point() noexcept {
    return reinterpret_cast<void*>(&allocate_handle_body);
}

void* commit_entry_point() noexcept {
    return reinterpret_cast<void*>(&commit_body);
}

void* allocate_record_entry_point() noexcept {
    return reinterpret_cast<void*>(&allocate_record_body);
}

void* set_registration_mode_entry_point() noexcept {
    return reinterpret_cast<void*>(&set_registration_mode_body);
}

} // namespace sunrise::client::hooks::entity_spawn
