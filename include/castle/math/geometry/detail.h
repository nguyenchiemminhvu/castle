#ifndef CASTLE_MATH_GEOMETRY_DETAIL_H
#define CASTLE_MATH_GEOMETRY_DETAIL_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/math/abs.h"
#include "castle/math/sqrt_real.h"
#include "castle/math/minmax.h"

namespace castle
{
namespace math
{
namespace detail
{

template <typename T>
using geometry_calc_type = typename meta::conditional<
    meta::is_integral<T>::value && (sizeof(T) <= sizeof(int32_t)),
    int64_t,
    T>::type;

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross2(T ax, T ay, T bx, T by) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * by - ay * bx);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot2(T ax, T ay, T bx, T by) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * bx + ay * by);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot3(
    T ax, T ay, T az,
    T bx, T by, T bz) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * bx + ay * by + az * bz);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross3_x(
    T ay, T az, T by, T bz) CASTLE_NOEXCEPT
{
    return static_cast<T>(ay * bz - az * by);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross3_y(
    T az, T ax, T bz, T bx) CASTLE_NOEXCEPT
{
    return static_cast<T>(az * bx - ax * bz);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross3_z(
    T ax, T ay, T bx, T by) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * by - ay * bx);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool near_zero(T value, T epsilon) CASTLE_NOEXCEPT
{
    return abs(value) <= epsilon;
}

} // namespace detail
} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_DETAIL_H
