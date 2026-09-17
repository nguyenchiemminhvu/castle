// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file hypot.hpp
 * @brief Overflow-resistant Euclidean norms for floating-point values.
 * @details Computes two-dimensional and three-dimensional vector magnitudes by
 * scaling each input by the largest absolute component before squaring. This
 * greatly reduces intermediate overflow risk compared with directly evaluating
 * `sqrt_real(x * x + y * y)` or `sqrt_real(x * x + y * y + z * z)`. The final
 * multiplication by the scale factor can still overflow if the true norm is
 * not representable, and non-finite inputs are not specially normalized.
 *
 * @code
 * #include "castle/math/hypot.hpp"
 *
 * constexpr float planar = castle::math::hypot(3.0f, 4.0f);
 * constexpr float spatial = castle::math::hypot(1.0f, 2.0f, 2.0f);
 * @endcode
 */

#ifndef CASTLE_MATH_HYPOT_HPP
#define CASTLE_MATH_HYPOT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle/algorithm/algorithm.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes the Euclidean norm of a 2D floating-point vector.
 * @tparam T Floating-point component type.
 * @param x First component.
 * @param y Second component.
 * @return `sqrt(x * x + y * y)` evaluated with scale normalization.
 * @note Returns zero when both inputs compare equal to zero.
 * @warning The scaling step reduces intermediate overflow risk but does not
 * guarantee a representable final result for extremely large finite inputs.
 */
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

/**
 * @brief Computes the Euclidean norm of a 3D floating-point vector.
 * @tparam T Floating-point component type.
 * @param x First component.
 * @param y Second component.
 * @param z Third component.
 * @return `sqrt(x * x + y * y + z * z)` evaluated with scale normalization.
 * @note Returns zero when all inputs compare equal to zero.
 * @warning The scaling step reduces intermediate overflow risk but does not
 * guarantee a representable final result for extremely large finite inputs.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
hypot(T x, T y, T z) CASTLE_NOEXCEPT
{
    CASTLE_CONST T ax = abs(x);
    CASTLE_CONST T ay = abs(y);
    CASTLE_CONST T az = abs(z);
    CASTLE_CONST T scale = castle::max(ax, castle::max(ay, az));

    if (scale == static_cast<T>(0))
    {
        return static_cast<T>(0);
    }

    CASTLE_CONST T sx = x / scale;
    CASTLE_CONST T sy = y / scale;
    CASTLE_CONST T sz = z / scale;
    return scale * sqrt_real(sx * sx + sy * sy + sz * sz);
}

}
}

#endif
