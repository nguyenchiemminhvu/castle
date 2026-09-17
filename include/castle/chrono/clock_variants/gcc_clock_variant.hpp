// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file gcc_clock_variant.hpp
 * @brief POSIX @c clock_gettime backend used by Castle chrono clock selection.
 *
 * Include this header directly only when testing or documenting the GCC-compatible
 * clock backend. In normal use, @c castle/chrono/clocks.hpp selects this file
 * automatically for GCC builds and for the current ARM, Clang, and default
 * fallbacks.
 *
 * Key constraints:
 * - Both helpers return nanosecond-resolution @c timespec values from POSIX clocks.
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
 *     const timespec realtime = castle::chrono::detail::clock_variant::realtime_ns();
 *     const timespec monotonic = castle::chrono::detail::clock_variant::monotonic_ns();
 *     (void)realtime;
 *     (void)monotonic;
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_CLOCK_VARIANTS_GCC_CLOCK_VARIANT_HPP
#define CASTLE_CHRONO_CLOCK_VARIANTS_GCC_CLOCK_VARIANT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

#include <time.h>

#define CASTLE_CHRONO_SYSTEM_CLOCK_GCC_VARIANT
#define CASTLE_CHRONO_STEADY_CLOCK_GCC_VARIANT

namespace castle
{
namespace chrono
{
namespace detail
{

/**
 * @brief Provides GCC-compatible POSIX clock queries for Castle chrono clocks.
 *
 * The returned @c timespec values are consumed by @c system_clock::now() and
 * @c steady_clock::now() when the GCC variant macros are active.
 */
struct clock_variant
{
    /**
     * @brief Reads the current wall-clock time from @c CLOCK_REALTIME.
     * @return A @c timespec whose seconds and nanoseconds describe the current realtime clock value.
     * @note The result is not monotonic and may move backward or forward if the platform clock is adjusted.
     * @warning Failure is reported through @c CASTLE_ASSERT; no exception is thrown.
     */
    static CASTLE_INLINE timespec realtime_ns() CASTLE_NOEXCEPT
    {
        timespec ts{};

        int result = clock_gettime(CLOCK_REALTIME, &ts);
        // The backend must surface a valid POSIX realtime timestamp for Castle clocks to function.
        CASTLE_ASSERT(result == 0, CASTLE_ERROR_GENERIC("clock_gettime(CLOCK_REALTIME) failed")); // LCOV_EXCL_BR_LINE

        return ts;
    }

    /**
     * @brief Reads the current monotonic time from @c CLOCK_MONOTONIC.
     * @return A @c timespec whose seconds and nanoseconds describe the current monotonic clock value.
     * @note Use this source for elapsed-time measurement because it is intended not to step backward during normal operation.
     * @warning Failure is reported through @c CASTLE_ASSERT; no exception is thrown.
     */
    static CASTLE_INLINE timespec monotonic_ns() CASTLE_NOEXCEPT
    {
        timespec ts{};

        int result = clock_gettime(CLOCK_MONOTONIC, &ts);
        // The backend must surface a valid POSIX monotonic timestamp for Castle clocks to function.
        CASTLE_ASSERT(result == 0, CASTLE_ERROR_GENERIC("clock_gettime(CLOCK_MONOTONIC) failed")); // LCOV_EXCL_BR_LINE

        return ts;
    }
};

}
}
}

#endif
