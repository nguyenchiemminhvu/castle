// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file knots.hpp
 * @brief Shared knot-table helpers for the table-based interpolators.
 * @details Defines the `extrapolation` policy enum and the two internal
 * helpers every 1-D table interpolator needs: validation that the abscissa
 * array is strictly increasing, and an `O(log n)` binary-search segment lookup.
 * Both are `constexpr`, so tables can be validated and queried at compile time.
 * `linear_table` and `cubic_spline` share this header
 * so the lookup and validation rules stay identical.
 *
 * @code
 * #include "castle/math/interpolation/knots.hpp"
 *
 * const float xs[] = {0.0F, 1.0F, 4.0F};
 * const castle::status s = castle::math::detail::validate_knots(xs, 3U); // ok
 * const castle::size_type i = castle::math::detail::find_segment(xs, 3U, 2.0F); // 1
 * @endcode
 */
#ifndef CASTLE_MATH_INTERPOLATION_KNOTS_HPP
#define CASTLE_MATH_INTERPOLATION_KNOTS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Behaviour of a table interpolator outside its knot range.
 */
enum class extrapolation : uint8_t
{
    clamp,  /**< Hold the nearest end value; the slope is zero. */
    linear  /**< Continue along the tangent line at the nearest end knot. */
};

namespace detail
{

/**
 * @brief Checks that a knot array can drive an interpolator.
 * @tparam T Floating-point type.
 * @param xs Abscissa values.
 * @param count Number of knots.
 * @return `status::ok` when `xs` is non-null, holds at least two knots and is
 *         strictly increasing; otherwise `status::invalid_argument`.
 * @note NaN entries fail the strict-increase test and are rejected.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR status validate_knots(CASTLE_CONST T* xs, size_type count) CASTLE_NOEXCEPT
{
    if ((xs == nullptr) || (count < 2U))
    {
        return status::invalid_argument;
    }

    for (size_type i = 1U; i < count; ++i)
    {
        if (!(xs[i - 1U] < xs[i]))
        {
            return status::invalid_argument;
        }
    }
    return status::ok;
}

/**
 * @brief Finds the segment that contains or is nearest to `x`.
 * @tparam T Floating-point type.
 * @param xs Strictly increasing abscissa values.
 * @param count Number of knots, at least 2.
 * @param x Query position.
 * @return Index `i` in `[0, count - 2]` such that `xs[i] <= x < xs[i + 1]`;
 *         queries below the table map to `0` and queries at or beyond the
 *         last knot map to `count - 2`. `O(log count)`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
size_type find_segment(CASTLE_CONST T* xs, size_type count, T x) CASTLE_NOEXCEPT
{
    // Invariant: the answer lies in [low, high - 1].
    size_type low = 0U;
    size_type high = count - 1U;
    while ((high - low) > 1U)
    {
        CASTLE_CONST size_type mid = low + ((high - low) / 2U);
        if (x < xs[mid])
        {
            high = mid;
        }
        else
        {
            low = mid;
        }
    }
    return low;
}

} // namespace detail
} // namespace math
} // namespace castle

#endif // CASTLE_MATH_INTERPOLATION_KNOTS_HPP
