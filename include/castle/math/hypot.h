#ifndef CASTLE_MATH_HYPOT_H
#define CASTLE_MATH_HYPOT_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/math/abs.h"
#include "castle/math/minmax.h"
#include "castle/math/sqrt_real.h"

namespace castle
{
namespace math
{

// ============================================================================
// Overflow-resistant Euclidean norm for two floating-point values.
//
// The largest magnitude is factored out before squaring. This is preferable to
// sqrt_real(x*x + y*y) when coordinates can be large.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
hypot(T x, T y) CASTLE_NOEXCEPT
{
    CASTLE_CONST T ax = abs(x);
    CASTLE_CONST T ay = abs(y);
    CASTLE_CONST T scale = (ax > ay) ? ax : ay;

    if (scale == static_cast<T>(0))
    {
        return static_cast<T>(0);
    }

    CASTLE_CONST T sx = x / scale;
    CASTLE_CONST T sy = y / scale;
    return scale * sqrt_real(sx * sx + sy * sy);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
hypot(T x, T y, T z) CASTLE_NOEXCEPT
{
    CASTLE_CONST T ax = abs(x);
    CASTLE_CONST T ay = abs(y);
    CASTLE_CONST T az = abs(z);
    CASTLE_CONST T scale = max(ax, max(ay, az));

    if (scale == static_cast<T>(0))
    {
        return static_cast<T>(0);
    }

    CASTLE_CONST T sx = x / scale;
    CASTLE_CONST T sy = y / scale;
    CASTLE_CONST T sz = z / scale;
    return scale * sqrt_real(sx * sx + sy * sy + sz * sz);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_HYPOT_H
