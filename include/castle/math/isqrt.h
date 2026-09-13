#ifndef CASTLE_MATH_ISQRT_H
#define CASTLE_MATH_ISQRT_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// Integer floor square root.
//
// Returns the largest integer r for which r*r <= value. Division is used for
// the comparison so the multiplication cannot overflow.
//
// Real-life use cases:
//   - Integer Euclidean distances when floating point is undesirable.
//   - Raster/grid dimensions and lookup-table sizing.
//   - Fixed-point geometry kernels.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_unsigned<T>::value, T>::type
isqrt(T value) CASTLE_NOEXCEPT
{
    if (value < static_cast<T>(2))
    {
        return value;
    }

    T low = static_cast<T>(1);
    T high = static_cast<T>(value / static_cast<T>(2) + static_cast<T>(1));

    while (low <= high)
    {
        const T middle = static_cast<T>(low + (high - low) / static_cast<T>(2));
        if (middle <= value / middle)
        {
            low = static_cast<T>(middle + static_cast<T>(1));
        }
        else
        {
            high = static_cast<T>(middle - static_cast<T>(1));
        }
    }
    return high;
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_signed<T>::value, T>::type
isqrt(T value) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(value >= T{0},
                  CASTLE_ERROR_GENERIC("castle::math::isqrt: negative input"));
    return isqrt(static_cast<typename meta::make_unsigned<T>::type>(value));
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_ISQRT_H
