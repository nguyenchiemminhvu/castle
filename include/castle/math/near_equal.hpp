// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file near_equal.hpp
 * @brief Hybrid absolute/relative floating-point comparison.
 *
 * This header provides an explicit-tolerance comparison for floating-point
 * values. Use it when equality decisions must tolerate measurement noise or
 * rounding error, and when the tolerance must be chosen by the caller rather
 * than hidden inside the library.
 *
 * Example:
 * @code
 * const bool same = castle::math::near_equal(1000.0f, 1000.5f, 0.001f);
 * const bool zeroish = castle::math::near_equal(0.0004f, 0.0f, 0.001f);
 * (void)same;
 * (void)zeroish;
 * @endcode
 *
 * @note The comparison uses `epsilon * max(1, abs(a), abs(b))`.
 */
#ifndef CASTLE_MATH_NEAR_EQUAL_HPP
#define CASTLE_MATH_NEAR_EQUAL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/abs.hpp"
#include "castle/algorithm/algorithm.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Compares two floating-point values using a caller-supplied tolerance.
 * @tparam T Floating-point type.
 * @param a First value.
 * @param b Second value.
 * @param epsilon Non-negative tolerance selected by the caller.
 * @return `true` when `abs(a - b) <= epsilon * max(1, abs(a), abs(b))`;
 *         otherwise `false`.
 * @note Values near zero use an absolute tolerance because the scaling term is
 *       clamped to at least `1`.
 * @warning A negative `epsilon` makes the comparison fail for all finite
 *          inputs, including exact matches.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, bool>::type
near_equal(T a, T b, T epsilon) CASTLE_NOEXCEPT
{
    CASTLE_CONST T difference = abs(static_cast<T>(a - b));
    CASTLE_CONST T scale = castle::max(static_cast<T>(1), castle::max(abs(a), abs(b)));
    return difference <= epsilon * scale;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_NEAR_EQUAL_HPP
