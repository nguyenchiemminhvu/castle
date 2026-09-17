#ifndef CASTLE_CHRONO_CLOCKS_H
#define CASTLE_CHRONO_CLOCKS_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/ratio.h"
#include "castle/chrono/time_point.h"

#if CASTLE_COMPILER_ARM
#include "castle/chrono/clock_variants/arm_clock_variant.h"
#elif CASTLE_COMPILER_CLANG
#include "castle/chrono/clock_variants/clang_clock_variant.h"
#elif CASTLE_COMPILER_GCC
#include "castle/chrono/clock_variants/gcc_clock_variant.h"
#else
#include "castle/chrono/clock_variants/default_clock_variant.h"
#endif

#include <stdint.h>
#include <time.h>

// ============================================================================
// Platform clock hooks
// ============================================================================
// CASTLE deliberately does not choose an OS, HAL, timer peripheral, or libc
// implementation for `now()`. The application owns that policy.
//
// Without the configuration macros, the library expects these two C-linkage
// functions to be provided by the platform layer:
//
//     int64_t castle_chrono_system_clock_now() noexcept;
//     int64_t castle_chrono_steady_clock_now() noexcept;
//
// Each function returns ticks in the corresponding clock's duration period.
// By default that period is nanoseconds. Defining the *_NOW macros before
// including this header replaces the function call with the supplied expression
// (for example `HAL_GetTick()` for a millisecond timer).
// ============================================================================
#ifndef CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API
extern "C" int64_t castle_chrono_system_clock_now() CASTLE_NOEXCEPT;
#endif

#ifndef CASTLE_CHRONO_STEADY_CLOCK_NOW_API
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

// The aliases below are kept outside the class declaration so a platform can
// override only the period without duplicating the clock implementation.

using system_clock_duration = duration<int64_t, CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD>;
using steady_clock_duration = duration<int64_t, CASTLE_CHRONO_STEADY_CLOCK_PERIOD>;

class system_clock
{
public:
    using duration = system_clock_duration;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = chrono::time_point<system_clock, duration>;

    static constexpr bool is_steady = false;

    static time_point now() CASTLE_NOEXCEPT
    {
#if defined(CASTLE_CHRONO_SYSTEM_CLOCK_GCC_VARIANT)
        struct timespec ts = detail::clock_variant::realtime_ns();
        return time_point(
            duration(
                static_cast<rep>(ts.tv_sec) * static_cast<rep>(period::den) +
                static_cast<rep>(ts.tv_nsec)
            )
        );
#elif defined(CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API)
        return time_point(duration(static_cast<rep>(CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API())));
#else // must find external function
        return time_point(duration(castle_chrono_system_clock_now()));
#endif
    }

    static ::time_t to_time_t(const time_point& value) CASTLE_NOEXCEPT
    {
        using time_t_duration = chrono::duration<int64_t, castle::math::ratio<1, 1> >;
        return static_cast<::time_t>(
            duration_cast<time_t_duration>(value.time_since_epoch()).count());
    }

    static time_point from_time_t(::time_t value) CASTLE_NOEXCEPT
    {
        using time_t_duration = chrono::duration<int64_t, castle::math::ratio<1, 1> >;
        return time_point(duration_cast<duration>(
            time_t_duration(static_cast<int64_t>(value))));
    }
};

// ============================================================================
// steady_clock
// ============================================================================
// Monotonic clock. The hook must be backed by a source that does not move
// backward during normal operation. Hardware counter wrap-around policy belongs
// to the platform hook; the chrono layer intentionally does not guess it.
// ============================================================================
class steady_clock
{
public:
    using duration = steady_clock_duration;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = chrono::time_point<steady_clock, duration>;

    static constexpr bool is_steady = true;

    static time_point now() CASTLE_NOEXCEPT
    {
#if defined(CASTLE_CHRONO_STEADY_CLOCK_GCC_VARIANT)
        struct timespec ts = detail::clock_variant::monotonic_ns();
        return time_point(
            duration(
                static_cast<rep>(ts.tv_sec) * static_cast<rep>(period::den) +
                static_cast<rep>(ts.tv_nsec)
            )
        );
#elif defined(CASTLE_CHRONO_STEADY_CLOCK_NOW_API)
        return time_point(duration(static_cast<rep>(CASTLE_CHRONO_STEADY_CLOCK_NOW_API())));
#else // must find external function
        return time_point(duration(castle_chrono_steady_clock_now()));
#endif
    }
};

// Pre-C++17 requires an out-of-class definition when is_steady is odr-used.
CASTLE_CONSTEXPR bool system_clock::is_steady;
CASTLE_CONSTEXPR bool steady_clock::is_steady;

// The project explicitly wants high_resolution_clock to be a system-clock
// variant. Keeping it an alias also guarantees its time_point type matches the
// system clock exactly and avoids a second platform backend.
using high_resolution_clock = system_clock;

} // namespace chrono
} // namespace castle

#endif // CASTLE_CHRONO_CLOCKS_H
