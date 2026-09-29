// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file gcc_clock_variant.hpp
 * @brief GCC-compatible system and steady clock backends for Castle chrono.
 *
 * Include this header directly only when testing or documenting the GCC-compatible
 * clock backend. In normal use, @c castle/chrono/clocks.hpp selects this file
 * automatically for GCC builds and for the current ARM, Clang, and default
 * fallbacks.
 *
 * Key constraints:
 * - Both helpers return nanosecond-resolution @c timespec values.
 * - @c realtime_ns() reflects wall-clock time and may jump if the system clock changes.
 * - @c monotonic_ns() is intended for monotonic elapsed-time measurement.
 * - Conversion to Castle durations happens in @c clocks.hpp and is subject to normal
 *   duration arithmetic overflow limits.
 *
 * Example:
 * @code
 * #include "castle/chrono/clock_variants/gcc_clock_variant.hpp"
 *
 * int main()
 * {
 *     const timespec realtime = castle::chrono::system_clock_adapter::realtime_ns();
 *     const timespec monotonic = castle::chrono::system_clock_adapter::monotonic_ns();
 *     (void)realtime;
 *     (void)monotonic;
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_CLOCK_VARIANTS_GCC_CLOCK_VARIANT_HPP
#define CASTLE_CHRONO_CLOCK_VARIANTS_GCC_CLOCK_VARIANT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/error/status.hpp"

#include <time.h>

#if CASTLE_USING_TIMEX
#include <sys/timex.h> // already include <sys/time.h>
extern "C" int clock_adjtime(clockid_t, struct timex*) CASTLE_NOEXCEPT;
#endif

#define CASTLE_CHRONO_SYSTEM_CLOCK_VARIANT
#define CASTLE_CHRONO_STEADY_CLOCK_VARIANT
#define CASTLE_CHRONO_SYSTEM_CLOCK_GCC_VARIANT
#define CASTLE_CHRONO_STEADY_CLOCK_GCC_VARIANT

namespace castle
{
namespace chrono
{
namespace detail
{

static int64_t scaled_frequency(int32_t frequency_ppb) CASTLE_NOEXCEPT
{
    CASTLE_CONST int64_t scaled = static_cast<int64_t>(frequency_ppb) * 65536LL;
    return scaled / 1000LL;
}

struct native_clock_api
{
#if CASTLE_USING_POSIX_APIS
    static int get_time(clockid_t clock_id, timespec* value) CASTLE_NOEXCEPT
    {
        return clock_gettime(clock_id, value);
    }

    static int set_time(clockid_t clock_id, timespec const* value) CASTLE_NOEXCEPT
    {
        return clock_settime(clock_id, value);
    }

#if CASTLE_USING_TIMEX
    static int adjust_time(clockid_t clock_id, struct timex* adjustment) CASTLE_NOEXCEPT
    {
        return clock_adjtime(clock_id, adjustment);
    }
#endif
#endif
};

#if CASTLE_USING_POSIX_APIS
template <typename ClockApi>
static castle::status step_clock(int64_t offset_nanoseconds) CASTLE_NOEXCEPT
{
    timespec current{};
    if (ClockApi::get_time(CLOCK_REALTIME, &current) != 0)
    {
        return castle::status::system_call_error;
    }

    int64_t seconds = static_cast<int64_t>(current.tv_sec);
    int64_t nanoseconds = static_cast<int64_t>(current.tv_nsec);
    CASTLE_CONST int64_t offset_seconds = offset_nanoseconds / 1000000000LL;
    nanoseconds += offset_nanoseconds % 1000000000LL;

    if ((offset_seconds > 0LL && seconds > INT64_MAX - offset_seconds)
     || (offset_seconds < 0LL && seconds < INT64_MIN - offset_seconds))
    {
        return castle::status::out_of_range;
    }
    seconds += offset_seconds;

    if (nanoseconds >= 1000000000LL)
    {
        if (seconds == INT64_MAX)
        {
            return castle::status::out_of_range;
        }
        ++seconds;
        nanoseconds -= 1000000000LL;
    }
    else if (nanoseconds < 0LL)
    {
        if (seconds == INT64_MIN)
        {
            return castle::status::out_of_range;
        }
        --seconds;
        nanoseconds += 1000000000LL;
    }

    timespec adjusted{};
    adjusted.tv_sec = static_cast<time_t>(seconds);
    // LCOV_EXCL_START
    if (static_cast<int64_t>(adjusted.tv_sec) != seconds)
    {
        return castle::status::out_of_range;
    }
    // LCOV_EXCL_STOP
    adjusted.tv_nsec = static_cast<long>(nanoseconds);

    return ClockApi::set_time(CLOCK_REALTIME, &adjusted) == 0
        ? castle::status::ok
        : castle::status::system_call_error;
}

#if CASTLE_USING_TIMEX
template <typename ClockApi>
static castle::status slew_clock(int32_t frequency_ppb) CASTLE_NOEXCEPT
{
    struct timex adjustment{};
    adjustment.modes = ADJ_FREQUENCY;
    adjustment.freq = static_cast<long>(scaled_frequency(frequency_ppb));
    return ClockApi::adjust_time(CLOCK_REALTIME, &adjustment) < 0
        ? castle::status::system_call_error
        : castle::status::ok;
}
#endif
#endif
}

/**
 * @brief Provides platform clock queries and adjustments for Castle chrono clocks.
 *
 * Linux builds use POSIX @c clock_gettime().
 */
class system_clock_adapter
{
public:
    /**
     * @brief Reads the current wall-clock time from @c CLOCK_REALTIME.
     * @return A @c timespec whose seconds and nanoseconds describe the current realtime clock value.
     * @note The result is not monotonic and may move backward or forward if the platform clock is adjusted.
     * @warning Failure is reported through @c CASTLE_ASSERT; no exception is thrown.
     */
    static CASTLE_INLINE timespec realtime_ns() CASTLE_NOEXCEPT
    {
    #if CASTLE_USING_POSIX_APIS
        timespec ts{};
        int result = clock_gettime(CLOCK_REALTIME, &ts);
        // The backend must surface a valid POSIX realtime timestamp for Castle clocks to function.
        CASTLE_ASSERT(result == 0, CASTLE_ERROR_GENERIC("clock_gettime(CLOCK_REALTIME) failed")); // LCOV_EXCL_BR_LINE
        return ts;
    #else
        CASTLE_ASSERT_FAIL(CASTLE_ERROR_GENERIC("no system clock read backend is configured"));
        return timespec{};
    #endif
    }

    /**
     * @brief Reads the current monotonic time from @c CLOCK_MONOTONIC.
     * @return A @c timespec whose seconds and nanoseconds describe the current monotonic clock value.
     * @note Use this source for elapsed-time measurement because it is intended not to step backward during normal operation.
     * @warning Failure is reported through @c CASTLE_ASSERT; no exception is thrown.
     */
    static CASTLE_INLINE timespec monotonic_ns() CASTLE_NOEXCEPT
    {
    #if CASTLE_USING_POSIX_APIS
        timespec ts{};
        int result = clock_gettime(CLOCK_MONOTONIC, &ts);
        // The backend must surface a valid POSIX monotonic timestamp for Castle clocks to function.
        CASTLE_ASSERT(result == 0, CASTLE_ERROR_GENERIC("clock_gettime(CLOCK_MONOTONIC) failed")); // LCOV_EXCL_BR_LINE
        return ts;
    #else
        CASTLE_ASSERT_FAIL(CASTLE_ERROR_GENERIC("no steady clock read backend is configured"));
        return timespec{};
    #endif
    }

    template <typename ClockApi = detail::native_clock_api>
    CASTLE_NODISCARD castle::status step(int64_t offset_nanoseconds) CASTLE_CONST CASTLE_NOEXCEPT
    {
    #if CASTLE_USING_POSIX_APIS
        return detail::step_clock<ClockApi>(offset_nanoseconds);
    #else
        (void)offset_nanoseconds;
        return castle::status::system_call_error;
    #endif
    }

    template <typename ClockApi = detail::native_clock_api>
    CASTLE_NODISCARD castle::status slew(int32_t frequency_ppb) CASTLE_CONST CASTLE_NOEXCEPT
    {
    #if CASTLE_USING_TIMEX
        return detail::slew_clock<ClockApi>(frequency_ppb);
    #else
        (void)frequency_ppb;
        return castle::status::system_call_error;
    #endif
    }
};

}
}

#endif
