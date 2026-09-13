#ifndef CASTLE_MATH_POWI_H
#define CASTLE_MATH_POWI_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// Integer exponentiation by squaring.
//
// The number of multiplications is O(log(exponent)), making this preferable to
// repeated multiplication in tight embedded loops and useful in constexpr code.
//
// Real-life use cases:
//   - Sensor scaling and calibration polynomials with integer powers.
//   - Fixed-point gain conversion.
//   - Compile-time generation of small lookup constants.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T powi(T base, unsigned exponent) CASTLE_NOEXCEPT
{
    T result = T{1};
    T factor = base;
    unsigned remaining = exponent;

    while (remaining != 0U)
    {
        if ((remaining & 1U) != 0U)
        {
            result = static_cast<T>(result * factor);
        }
        remaining >>= 1U;
        if (remaining != 0U)
        {
            factor = static_cast<T>(factor * factor);
        }
    }
    return result;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_POWI_H
