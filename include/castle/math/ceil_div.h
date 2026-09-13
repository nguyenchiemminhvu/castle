#ifndef CASTLE_MATH_CEIL_DIV_H
#define CASTLE_MATH_CEIL_DIV_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// Integer ceiling division for a positive denominator.
//
// Unlike (a + b - 1) / b, this formulation does not overflow when a is near
// the maximum representable value.
//
// Real-life use cases:
//   - Number of DMA transfers required for a buffer.
//   - Number of CAN/FlexRay frames required for a payload.
//   - Flash pages needed to store a fixed-size record set.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
ceil_div(T numerator, T denominator) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(denominator > T{0},
                  CASTLE_ERROR_GENERIC("castle::math::ceil_div: denominator must be positive"));

    CASTLE_CONST T quotient = numerator / denominator;
    CASTLE_CONST T remainder = numerator % denominator;

    return (remainder != T{0} && numerator > T{0})
               ? static_cast<T>(quotient + T{1})
               : quotient;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_CEIL_DIV_H
