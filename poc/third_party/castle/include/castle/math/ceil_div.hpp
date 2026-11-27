// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ceil_div.hpp
 * @brief Mathematical ceiling division for integral types.
 * @details Computes `ceil(numerator / denominator)` for integral operands when
 * the denominator is strictly positive. The implementation avoids the common
 * `(a + b - 1) / b` rewrite so callers do not risk overflow from the
 * intermediate addition near the type maximum. Negative numerators are handled
 * according to true mathematical ceiling, not raw C++ truncation rules.
 *
 * @code
 * #include "castle/math/ceil_div.hpp"
 *
 * constexpr int pages = castle::math::ceil_div(10, 4);
 * constexpr int signed_bucket = castle::math::ceil_div(-7, 3);
 * @endcode
 */

#ifndef CASTLE_MATH_CEIL_DIV_HPP
#define CASTLE_MATH_CEIL_DIV_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Divides two integers and rounds the result toward positive infinity.
 * @tparam T Integral type shared by both operands.
 * @param numerator Dividend to divide.
 * @param denominator Positive divisor.
 * @return The mathematical ceiling of `numerator / denominator`.
 * @note The result differs from ordinary C++ integer division only when the
 * quotient is positive and has a non-zero remainder.
 * @warning `denominator` must be greater than zero; zero or negative
 * denominators violate the function contract and trigger Castle's error
 * handling when checks are enabled.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
ceil_div(T numerator, T denominator) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(denominator > T{0}, // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::ceil_div: denominator must be positive"));

    CASTLE_CONST T quotient = numerator / denominator;
    CASTLE_CONST T remainder = numerator % denominator;

    return (remainder != T{0} && numerator > T{0})
               ? static_cast<T>(quotient + T{1})
               : quotient;
}

}
}

#endif
