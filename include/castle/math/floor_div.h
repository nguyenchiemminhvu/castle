#ifndef CASTLE_MATH_FLOOR_DIV_H
#define CASTLE_MATH_FLOOR_DIV_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// Integer floor division for a positive denominator.
//
// C++ integer division truncates toward zero. floor_div() gives mathematical
// floor(), which is the useful operation for signed grid/cell coordinates.
//
// Real-life use cases:
//   - Mapping signed world coordinates to map cells.
//   - Fixed-point quantization around zero.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
floor_div(T numerator, T denominator) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(denominator > T{0},
                  CASTLE_ERROR_GENERIC("castle::math::floor_div: denominator must be positive"));

    const T quotient = numerator / denominator;
    const T remainder = numerator % denominator;

    return (remainder != T{0} && numerator < T{0})
               ? static_cast<T>(quotient - T{1})
               : quotient;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_FLOOR_DIV_H
