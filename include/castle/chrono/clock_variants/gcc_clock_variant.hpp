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

struct clock_variant
{
    static CASTLE_INLINE timespec realtime_ns() CASTLE_NOEXCEPT
    {
        timespec ts{};

        int result = clock_gettime(CLOCK_REALTIME, &ts);
        CASTLE_ASSERT(result == 0, CASTLE_ERROR_GENERIC("clock_gettime(CLOCK_REALTIME) failed")); // LCOV_EXCL_BR_LINE

        return ts;
    }

    static CASTLE_INLINE timespec monotonic_ns() CASTLE_NOEXCEPT
    {
        timespec ts{};

        int result = clock_gettime(CLOCK_MONOTONIC, &ts);
        CASTLE_ASSERT(result == 0, CASTLE_ERROR_GENERIC("clock_gettime(CLOCK_MONOTONIC) failed")); // LCOV_EXCL_BR_LINE

        return ts;
    }
};

} // namespace detail
} // namespace chrono
} // namespace castle

#endif // CASTLE_CHRONO_CLOCK_VARIANTS_GCC_CLOCK_VARIANT_HPP
