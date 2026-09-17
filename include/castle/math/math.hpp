// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file math.hpp
 * @brief Umbrella include for Castle math facilities.
 *
 * This header aggregates the Castle math module so applications can include a
 * single file when broad access is more convenient than fine-grained includes.
 * It also provides lightweight floating-point helpers for epsilon-based
 * equality and zero checks.
 *
 * Example:
 * @code
 * #include "castle/math/math.hpp"
 *
 * static_assert(castle::sqrt<81U>::value == 9U, "integer sqrt");
 * const bool equal = castle::math::is_equal(1.0f, 1.0f);
 * const int clamped = castle::math::clamp(150, 0, 100);
 * (void)equal;
 * (void)clamped;
 * @endcode
 *
 * @note Prefer narrower headers when compile-time dependencies must stay small.
 */
#ifndef CASTLE_MATH_MATH_HPP
#define CASTLE_MATH_MATH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/constants.hpp"

#include "castle/math/fib.hpp"
#include "castle/math/gcd.hpp"
#include "castle/math/lcm.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/sqrt.hpp"
#include "castle/math/clamp.hpp"
#include "castle/math/mean.hpp"
#include "castle/math/ratio.hpp"
#include "castle/math/invert.hpp"
#include "castle/math/logarithm.hpp"
#include "castle/math/random.hpp"
#include "castle/math/ceil_div.hpp"
#include "castle/math/floor_div.hpp"
#include "castle/math/hypot.hpp"
#include "castle/math/factorial.hpp"
#include "castle/math/is_power_of_two.hpp"
#include "castle/math/isqrt.hpp"
#include "castle/math/lerp.hpp"
#include "castle/math/mod.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/powi.hpp"
#include "castle/math/saturating.hpp"
#include "castle/math/sign.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle/math/square.hpp"
#include "castle/math/geometry.hpp"
#include "castle/math/linalg.hpp"

#include <math.h>

namespace castle
{
namespace math
{

/**
 * @brief Tests whether two floating-point values differ by no more than one
 *        machine epsilon.
 * @tparam T Floating-point type.
 * @param a First value.
 * @param b Second value.
 * @return `true` when the values are exactly equal or their absolute
 *         difference is within `meta::floating_epsilon<T>::value`;
 *         otherwise `false`.
 * @note This uses an absolute epsilon test. For caller-selected tolerances or
 *       scale-aware comparisons, use `near_equal()`.
 * @warning Only floating-point types are supported.
 */
template <typename T>
CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_floating_point<T>::value, bool>
is_equal(T a, T b) CASTLE_NOEXCEPT
{
    return (a == b) || (castle::math::abs(a - b) <= meta::floating_epsilon<T>::value); // LCOV_EXCL_BR_LINE
}

/**
 * @brief Tests whether a floating-point value is close enough to zero to be
 *        treated as zero.
 * @tparam T Floating-point type.
 * @param a Value to test.
 * @return `true` when `abs(a)` is no greater than
 *         `meta::floating_epsilon<T>::value`; otherwise `false`.
 * @note This is a deterministic absolute-epsilon check with no hidden state.
 * @warning Only floating-point types are supported.
 */
template <typename T>
CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_floating_point<T>::value, bool>
is_zero(T a) CASTLE_NOEXCEPT
{
    return castle::math::abs(a) <= meta::floating_epsilon<T>::value;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_MATH_HPP