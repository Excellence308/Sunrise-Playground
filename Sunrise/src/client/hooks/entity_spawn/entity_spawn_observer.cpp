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

/** Initial load emits about 112 failures; this retains two complete passes without unbounded logs. */
constexpr std::uint32_t kFailureLimit = 256;
constexpr std::size_t kLineCapacity = 320;

struct TraceContext {
    std::uint32_t depth{};
    std::int32_t sourceHandle{-1};
    std::int32_t createArgument{-1};
    std::int32_t createMode{-1};
    std::int32_t simulationHandle{-1};
    bool allocateHandleCalled{};
    bool allocateHandleSucceeded{};
    bool commitCalled{};
    bool commitSucceeded{};
    bool allocateRecordCalled{};
    bool allocateRecordSucceeded{};
    bool registrationCalled{};
    bool registrationSucceeded{};
};

thread_local TraceContext g_trace;
std::atomic_uint32_t g_failureCount{};

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

/** @return The narrowest native stage proven by the nested observer results. */
[[nodiscard]] std::string_view failure_reason() noexcept {
    if (!g_trace.allocateHandleCalled || !g_trace.allocateHandleSucceeded) {
        return "handle_pool";
    }
    if (!g_trace.commitCalled) {
        return "commit_not_reached";
    }
    if (!g_trace.allocateRecordCalled || !g_trace.allocateRecordSucceeded) {
        return "record_pool";
    }
    if (!g_trace.registrationCalled) {
        return "buffer_setup";
    }
    if (!g_trace.registrationSucceeded) {
        return "registration";
    }
    if (!g_trace.commitSucceeded) {
        return "commit_unknown";
    }
    return "create_unknown";
}

/** Emits one bounded failure event after the original top-level routine has returned. */
void report_failure() noexcept {
    if (g_failureCount.fetch_add(1, std::memory_order_relaxed) >= kFailureLimit) {
        return;
    }
    const std::string_view reason = failure_reason();
    std::array<char, kLineCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=sobject_trace stage=create result=fail reason=%.*s "
                                      "source=0x%08X create_arg=%d create_mode=%d sim=0x%08X "
                                      "commit=%u record=%u registration=%u",
                                      static_cast<int>(reason.size()),
                                      reason.data(),
                                      static_cast<std::uint32_t>(g_trace.sourceHandle),
                                      g_trace.createArgument,
                                      g_trace.createMode,
                                      static_cast<std::uint32_t>(g_trace.simulationHandle),
                                      g_trace.commitCalled ? 1U : 0U,
                                      g_trace.allocateRecordSucceeded ? 1U : 0U,
                                      g_trace.registrationSucceeded ? 1U : 0U);
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
    }
    const auto call = original<Create>(Slot::create);
    std::int32_t* const result =
        call != nullptr ? call(manager, output, createArgument, sourceHandle, createMode) : output;
    if (outer) {
        if (read_output(result != nullptr ? result : output) == -1) {
            report_failure();
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
