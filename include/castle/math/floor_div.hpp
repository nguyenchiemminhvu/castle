// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file floor_div.hpp
 * @brief Mathematical floor division for integral types.
 * @details Computes `floor(numerator / denominator)` for integral operands
 * when the denominator is strictly positive. This differs from ordinary C++
 * integer division for negative, non-exact quotients because C++ truncates
 * toward zero. Use this helper when negative coordinates or signed buckets
 * must map to the mathematically lower cell.
 *
 * @code
 * #include "castle/math/floor_div.hpp"
 *
 * constexpr int left_cell = castle::math::floor_div(-7, 3);
 * constexpr int exact_cell = castle::math::floor_div(12, 3);
 * @endcode
 */

#ifndef CASTLE_MATH_FLOOR_DIV_HPP
#define CASTLE_MATH_FLOOR_DIV_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Divides two integers and rounds the result toward negative infinity.
 * @tparam T Integral type shared by both operands.
 * @param numerator Dividend to divide.
 * @param denominator Positive divisor.
 * @return The mathematical floor of `numerator / denominator`.
 * @note The result differs from ordinary C++ integer division only when the
 * quotient is negative and has a non-zero remainder.
 * @warning `denominator` must be greater than zero; zero or negative
 * denominators violate the function contract and trigger Castle's error
 * handling when checks are enabled.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
floor_div(T numerator, T denominator) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(denominator > T{0}, // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::floor_div: denominator must be positive"));

    CASTLE_CONST T quotient = numerator / denominator;
    CASTLE_CONST T remainder = numerator % denominator;

    return (remainder != T{0} && numerator < T{0})
               ? static_cast<T>(quotient - T{1})
               : quotient;
}

}
}

#endif
