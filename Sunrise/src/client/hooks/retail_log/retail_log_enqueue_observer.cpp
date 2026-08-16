#include "retail_log_enqueue_observer.h"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <intrin.h>
#include <string_view>

#include "../../../core/logging/log.h"
#include "../../targets/game.h"

namespace sunrise::client::hooks::retail_log {
namespace {

using Enqueue = void(__fastcall*)(std::int32_t, const char*) noexcept;
using SetCategoryVerbosity = void(__fastcall*)(std::int32_t, std::uint32_t) noexcept;

/** The game copies exactly this many bytes out of the caller's text buffer. */
constexpr std::size_t kNativeTextSize = 320;
/** Site id the game uses for an unregistered line. */
constexpr std::int32_t kUnregisteredSite = -1;
/** Line storage holds the cleaned text plus its fixed key prefix. */
constexpr std::size_t kEventCapacity = kNativeTextSize + 64;
/** A late config load resets the thresholds, so set them again on this period. A count will not
 *  do: a closed category emits fewer lines, so it advances slower and stays closed. */
constexpr std::uint64_t kReassertIntervalMs = 2'000;
/** How many categories the game's own verbosity table holds. */
constexpr std::uint32_t kCategoryCount = 26;
/** 0 is the game's loosest category threshold. A higher value logs less. */
constexpr std::uint32_t kMostVerbose = 0;

thread_local bool g_inObserver{};
/** Tick at which the next re-assert is due. Zero makes the first call assert. */
volatile LONG64 g_nextAssertTick{};
std::atomic_bool g_taskNineOriginReported{false};
std::atomic_bool g_sobjectFailureOriginReported{false};

/** Site id of the initial-slice task-start line in this Shadowkeep image. */
constexpr std::int32_t kTaskStartSite = 106;
/** Exact native line emitted when the successful destination schedules task 9. */
constexpr std::string_view kTaskNineStart =
    "world_controller:task_manager: Started   task 'ENUM(9)'.";
/** Site id of the generic simulation-entity creation failure in this Shadowkeep image. */
constexpr std::int32_t kEntityFailureSite = 193;
/** Exact native line emitted when one static/simulation object cannot be instantiated. */
constexpr std::string_view kSobjectFailure =
    "networking:simulation:entity: failed to create 'sobject' entity";
/** Frames kept from one matched native log call. */
constexpr std::size_t kStackFrameCapacity = 16;
/** This Shadowkeep image ends below RVA 0x09000000. */
constexpr std::size_t kGameImageRvaLimit = 0x09000000;

/**
 * Tests one exact registered line without trusting the native buffer length.
 * @param siteId Registered retail-log site id.
 * @param text Borrowed native line.
 * @param expectedSite Expected registered site.
 * @param expected Exact expected text.
 * @return True only for the expected line.
 */
[[nodiscard]] bool matches_line(std::int32_t siteId,
                                const char* text,
                                std::int32_t expectedSite,
                                std::string_view expected) noexcept {
    if (siteId != expectedSite || text == nullptr) {
        return false;
    }
    __try {
        for (std::size_t index = 0; index < expected.size(); ++index) {
            if (text[index] != expected[index]) {
                return false;
            }
        }
        return text[expected.size()] == '\0';
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

/**
 * Records the native origin of one exact line once per process.
 * This is diagnostic only: it identifies the exact game code path for static analysis.
 * @param siteId Registered retail-log site id.
 * @param text Borrowed native line.
 * @param caller Native return address left by the call into the enqueue funnel.
 * @param expectedSite Expected registered site.
 * @param expected Exact expected text.
 * @param stage Stable event name.
 * @param reported One-shot publication guard.
 */
void capture_origin(std::int32_t siteId,
                    const char* text,
                    const void* caller,
                    std::int32_t expectedSite,
                    std::string_view expected,
                    std::string_view stage,
                    std::atomic_bool& reported) noexcept {
    if (!matches_line(siteId, text, expectedSite, expected) || caller == nullptr
        || reported.exchange(true, std::memory_order_relaxed)) {
        return;
    }
    const auto* const base = reinterpret_cast<const std::byte*>(GetModuleHandleW(nullptr));
    const auto* const address = static_cast<const std::byte*>(caller);
    if (base == nullptr || address < base) {
        return;
    }
    std::array<void*, kStackFrameCapacity> frames{};
    const USHORT frameCount = RtlCaptureStackBackTrace(
        0, static_cast<ULONG>(frames.size()), frames.data(), nullptr);
    std::array<char, kEventCapacity> line{};
    int written = std::snprintf(line.data(),
                                line.size(),
                                "ev=retail_origin stage=%.*s site=%d caller_rva=0x%zX stack=",
                                static_cast<int>(stage.size()),
                                stage.data(),
                                siteId,
                                static_cast<std::size_t>(address - base));
    for (USHORT index = 0; written > 0 && index < frameCount
                           && static_cast<std::size_t>(written) < line.size();
         ++index) {
        const auto* const frame = static_cast<const std::byte*>(frames[index]);
        if (frame < base || static_cast<std::size_t>(frame - base) >= kGameImageRvaLimit) {
            continue;
        }
        const int appended = std::snprintf(line.data() + written,
                                           line.size() - static_cast<std::size_t>(written),
                                           "%s0x%zX",
                                           line[written - 1] == '=' ? "" : ",",
                                           static_cast<std::size_t>(frame - base));
        written = appended > 0 ? written + appended : 0;
    }
    if (written > 0 && static_cast<std::size_t>(written) < line.size()) {
        core::log::write(core::log::Channel::client,
                         core::log::Level::info,
                         {line.data(), static_cast<std::size_t>(written)});
    }
}

/**
 * Copies the native text into fixed storage as one printable line.
 * @param text Borrowed native buffer.
 * @param output Receives the cleaned characters.
 * @return Number of characters written.
 */
[[nodiscard]] std::size_t sanitize(const char* text, std::array<char, kNativeTextSize>& output) {
    std::size_t length = 0;
    __try {
        for (; length < kNativeTextSize - 1 && text[length] != '\0'; ++length) {
            const char value = text[length];
            // One line, one event: the native text carries its own line breaks.
            output[length] = value >= ' ' && value != '\x7F' ? value : ' ';
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    while (length != 0 && output[length - 1] == ' ') {
        --length;
    }
    return length;
}

/**
 * Writes one captured line.
 * @param siteId Registered site id.
 * @param text Borrowed native buffer.
 */
void capture_line(std::int32_t siteId, const char* text) noexcept {
    if (!core::log::accepts(core::log::Channel::client, core::log::Level::info)) {
        return;
    }
    std::array<char, kNativeTextSize> sanitized{};
    const std::size_t textLength = sanitize(text, sanitized);
    std::array<char, kEventCapacity> line{};
    const int written = std::snprintf(line.data(),
                                      line.size(),
                                      "ev=retail site=%d text=%.*s",
                                      siteId,
                                      static_cast<int>(textLength),
                                      sanitized.data());
    if (written <= 0) {
        return;
    }
    const auto length = static_cast<std::size_t>(written) < line.size()
                            ? static_cast<std::size_t>(written)
                            : line.size() - 1;
    core::log::write(core::log::Channel::client, core::log::Level::info, {line.data(), length});
}

/**
 * Mirrors the single funnel every retail log line passes through.
 * @param siteId Registered site id.
 * @param text Native buffer holding the already-formatted line.
 */
__declspec(noinline) void __fastcall enqueue_body(std::int32_t siteId, const char* text) noexcept {
    const void* const caller = _ReturnAddress();
    // The verbosity setter logs through this same funnel; without this it would recurse.
    const bool outer = !g_inObserver;
    g_inObserver = true;
    const auto call = reinterpret_cast<Enqueue>(g_handle.original);
    if (call != nullptr) {
        call(siteId, text);
    }
    if (outer) {
        if (siteId != kUnregisteredSite && text != nullptr) {
            capture_line(siteId, text);
            capture_origin(siteId,
                           text,
                           caller,
                           kTaskStartSite,
                           kTaskNineStart,
                           "task_9",
                           g_taskNineOriginReported);
            capture_origin(siteId,
                           text,
                           caller,
                           kEntityFailureSite,
                           kSobjectFailure,
                           "sobject_create_failure",
                           g_sobjectFailureOriginReported);
        }
        assert_verbosity();
        g_inObserver = false;
    }
}

} // namespace

/** @return The enqueue observer body itself, with internal linkage. */
void* enqueue_entry_point() noexcept {
    return reinterpret_cast<void*>(&enqueue_body);
}

/**
 * Opens every category in the game's own log table, once we know the block exists. Reaching the
 * enqueue funnel is the proof: without the block the native body returns early.
 */
void assert_verbosity() noexcept {
    // How much the game logs follows the client threshold, so debug is what opens its table.
    if (!core::log::accepts(core::log::Channel::client, core::log::Level::debug)) {
        return;
    }
    const auto now = static_cast<LONG64>(GetTickCount64());
    const LONG64 due = g_nextAssertTick;
    if (now < due) {
        return;
    }
    // One claim per period, so concurrent funnel threads do not all reopen the table.
    if (InterlockedCompareExchange64(
            &g_nextAssertTick, now + static_cast<LONG64>(kReassertIntervalMs), due)
        != due) {
        return;
    }
    const auto setter = reinterpret_cast<SetCategoryVerbosity>(
        targets::game::retail_log::get().setCategoryVerbosity);
    if (setter == nullptr) {
        return;
    }
    for (std::uint32_t category = 0; category < kCategoryCount; ++category) {
        setter(static_cast<std::int32_t>(category), kMostVerbose);
    }
}

} // namespace sunrise::client::hooks::retail_log
