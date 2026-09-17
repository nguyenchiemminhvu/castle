// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file clocks.hpp
 * @brief Platform-configurable wall-clock and monotonic clock types for Castle chrono.
 *
 * Include this header when code needs current time points instead of manually
 * constructing durations or time points. Compiler detection in
 * @c castle/core/compiler.hpp selects one of the clock-variant headers in this
 * order: ARM, Clang, GCC, then the default fallback. All currently selected
 * variants reuse the GCC-compatible POSIX @c clock_gettime backend unless a
 * project overrides the source with the @c CASTLE_CHRONO_*_NOW_API macros or
 * provides the required external C functions.
 *
 * Key constraints:
 * - The default tick resolution for both clocks is nanoseconds unless
 *   @c CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD or @c CASTLE_CHRONO_STEADY_CLOCK_PERIOD is overridden.
 * - @c system_clock represents wall-clock time and is not monotonic.
 * - @c steady_clock is intended to be monotonic; correct monotonicity depends on the selected backend or platform hook.
 * - Tick conversion and time-point arithmetic do not check overflow.
 *
 * Example:
 * @code
 * #include "castle/chrono/clocks.hpp"
 *
 * int main()
 * {
 *     const auto start = castle::chrono::steady_clock::now();
 *     const auto wall = castle::chrono::system_clock::from_time_t(1);
 *     (void)start;
 *     (void)wall;
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_CLOCKS_HPP
#define CASTLE_CHRONO_CLOCKS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/ratio.hpp"
#include "castle/chrono/time_point.hpp"

#if CASTLE_COMPILER_ARM
#include "castle/chrono/clock_variants/arm_clock_variant.hpp"
#elif CASTLE_COMPILER_CLANG
#include "castle/chrono/clock_variants/clang_clock_variant.hpp"
#elif CASTLE_COMPILER_GCC
#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"
#else
#include "castle/chrono/clock_variants/default_clock_variant.hpp"
#endif

#include <stdint.h>
#include <time.h>

#ifndef CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API
/**
 * @brief Returns the current system-clock tick count from the platform layer.
 * @return The current wall-clock value expressed in @c CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD ticks.
 * @note Provide this function only when @c CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API is not defined.
 * @warning The return value is interpreted exactly as configured by @c CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD.
 */
extern "C" int64_t castle_chrono_system_clock_now() CASTLE_NOEXCEPT;
#endif

#ifndef CASTLE_CHRONO_STEADY_CLOCK_NOW_API
/**
 * @brief Returns the current steady-clock tick count from the platform layer.
 * @return The current monotonic tick value expressed in @c CASTLE_CHRONO_STEADY_CLOCK_PERIOD ticks.
 * @note Provide this function only when @c CASTLE_CHRONO_STEADY_CLOCK_NOW_API is not defined.
 * @warning The platform hook is responsible for monotonic behavior and any hardware counter wrap policy.
 */
extern "C" int64_t castle_chrono_steady_clock_now() CASTLE_NOEXCEPT;
#endif

#ifndef CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD
#define CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD castle::nano
#endif

#ifndef CASTLE_CHRONO_STEADY_CLOCK_PERIOD
#define CASTLE_CHRONO_STEADY_CLOCK_PERIOD castle::nano
#endif

namespace castle
{
namespace chrono
{

/**
 * @brief Duration type used by @c system_clock.
 * @note The period defaults to nanoseconds and can be overridden with @c CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD.
 */
using system_clock_duration = duration<int64_t, CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD>;

/**
 * @brief Duration type used by @c steady_clock.
 * @note The period defaults to nanoseconds and can be overridden with @c CASTLE_CHRONO_STEADY_CLOCK_PERIOD.
 */
using steady_clock_duration = duration<int64_t, CASTLE_CHRONO_STEADY_CLOCK_PERIOD>;

/**
 * @brief Wall-clock time source for calendar-style timestamps and external time interchange.
 *
 * This clock may move backward or forward if the underlying platform time is adjusted.
 * Use @c steady_clock instead for measuring elapsed time.
 */
class system_clock
{
public:
    using duration = system_clock_duration;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = chrono::time_point<system_clock, duration>;

    /**
     * @brief Indicates whether the clock is monotonic.
     * @note @c system_clock is intentionally not steady because wall-clock time can change discontinuously.
     */
    static CASTLE_CONSTEXPR bool is_steady = false;

    /**
     * @brief Reads the current wall-clock time.
     * @return A @c system_clock::time_point whose tick count is expressed in @c system_clock::duration units.
     * @note The selected source is, in order of precedence, the GCC-compatible variant macro path,
     *       the @c CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API macro expression, or the external C hook.
     * @warning The returned value is only as accurate and stable as the supplied platform clock source.
     */
    static time_point now() CASTLE_NOEXCEPT
    {
#if defined(CASTLE_CHRONO_SYSTEM_CLOCK_GCC_VARIANT)
        struct timespec ts = detail::clock_variant::realtime_ns();
        return time_point(
            duration(
                // Convert whole seconds to clock ticks, then add the remaining nanoseconds field.
                static_cast<rep>(ts.tv_sec) * static_cast<rep>(period::den) +
                static_cast<rep>(ts.tv_nsec)
            )
        );
#elif defined(CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API)
        return time_point(duration(static_cast<rep>(CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API())));
#else
        return time_point(duration(castle_chrono_system_clock_now()));
#endif
    }

    /**
     * @brief Converts a Castle system-clock time point to C @c time_t seconds.
     * @param value Time point to convert.
     * @return The number of whole seconds since the system-clock epoch.
     * @note Conversion uses @c duration_cast and therefore truncates toward zero when the clock period is finer than one second.
     * @warning Fractional ticks smaller than one second are discarded.
     */
    static ::time_t to_time_t(CASTLE_CONST time_point& value) CASTLE_NOEXCEPT
    {
        using time_t_duration = chrono::duration<int64_t, castle::math::ratio<1, 1> >;
        return static_cast<::time_t>(
            duration_cast<time_t_duration>(value.time_since_epoch()).count());
    }

    /**
     * @brief Converts C @c time_t seconds to a Castle system-clock time point.
     * @param value Whole seconds since the system-clock epoch.
     * @return A @c system_clock::time_point expressed in @c system_clock::duration ticks.
     * @note Conversion uses @c duration_cast, so the returned tick count matches the configured system-clock period.
     * @warning Large inputs may overflow the target duration representation.
     */
    static time_point from_time_t(::time_t value) CASTLE_NOEXCEPT
    {
        using time_t_duration = chrono::duration<int64_t, castle::math::ratio<1, 1> >;
        return time_point(duration_cast<duration>(
            time_t_duration(static_cast<int64_t>(value))));
    }
};

/**
 * @brief Monotonic clock for elapsed-time measurement, timeouts, and scheduling.
 *
 * The platform hook or selected backend must provide a source that does not move
 * backward during normal operation. Wrap-around handling remains the platform's responsibility.
 */
class steady_clock
{
public:
    using duration = steady_clock_duration;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = chrono::time_point<steady_clock, duration>;

    /**
     * @brief Indicates whether the clock is monotonic.
     * @note @c steady_clock is defined as steady and callers should rely on it for elapsed-time calculations.
     */
    static CASTLE_CONSTEXPR bool is_steady = true;

    /**
     * @brief Reads the current monotonic time.
     * @return A @c steady_clock::time_point whose tick count is expressed in @c steady_clock::duration units.
     * @note The selected source is, in order of precedence, the GCC-compatible variant macro path,
     *       the @c CASTLE_CHRONO_STEADY_CLOCK_NOW_API macro expression, or the external C hook.
     * @warning If the supplied backend is not actually monotonic, elapsed-time calculations become unreliable.
     */
    static time_point now() CASTLE_NOEXCEPT
    {
#if defined(CASTLE_CHRONO_STEADY_CLOCK_GCC_VARIANT)
        struct timespec ts = detail::clock_variant::monotonic_ns();
        return time_point(
            duration(
                // Convert whole seconds to clock ticks, then add the remaining nanoseconds field.
                static_cast<rep>(ts.tv_sec) * static_cast<rep>(period::den) +
                static_cast<rep>(ts.tv_nsec)
            )
        );
#elif defined(CASTLE_CHRONO_STEADY_CLOCK_NOW_API)
        return time_point(duration(static_cast<rep>(CASTLE_CHRONO_STEADY_CLOCK_NOW_API())));
#else
        return time_point(duration(castle_chrono_steady_clock_now()));
#endif
    }
};

CASTLE_CONSTEXPR bool system_clock::is_steady;
CASTLE_CONSTEXPR bool steady_clock::is_steady;

/**
 * @brief Highest-resolution clock alias exposed by this Castle chrono implementation.
 * @note This library intentionally aliases @c high_resolution_clock to @c system_clock so both names share the same epoch and time-point type.
 */
using high_resolution_clock = system_clock;

}
}

#endif
