// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file clamp.hpp
 * @brief Inclusive range limiting for arithmetic values.
 * @details Returns the nearest value inside `[low, high]` without depending on
 * STL algorithms. This helper is useful for saturating configuration values,
 * control outputs, and sensor-derived quantities before they are consumed by
 * hardware-facing code. The function assumes the caller provides a valid range
 * with `low <= high`.
 *
 * @code
 * #include "castle/math/clamp.hpp"
 *
 * constexpr int duty = castle::math::clamp(125, 0, 100);
 * constexpr int centered = castle::math::clamp(-5, -2, 2);
 * @endcode
 */

#ifndef CASTLE_MATH_CLAMP_HPP
#define CASTLE_MATH_CLAMP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Clamps a value to an inclusive lower and upper bound.
 * @tparam T Arithmetic type shared by the value and both bounds.
 * @param value Candidate value to constrain.
 * @param low Inclusive lower bound.
 * @param high Inclusive upper bound.
 * @return `low` when `value < low`, `high` when `value > high`, otherwise
 * `value`.
 * @note This is an `O(1)` selection with no hidden allocation or exception
 * paths.
 * @warning `low` must not exceed `high`; violating this precondition triggers
 * Castle's error handling when checks are enabled.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_arithmetic<T>::value, T>::type
clamp(T value, T low, T high) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(low <= high, "clamp precondition violated: low > hi.hpp"); // LCOV_EXCL_BR_LINE
    return (value < low) ? low : ((high < value) ? high : value); // LCOV_EXCL_BR_LINE
}

}
}

#endif
