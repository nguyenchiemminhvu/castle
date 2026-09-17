#ifndef CASTLE_MATH_NEAR_EQUAL_H
#define CASTLE_MATH_NEAR_EQUAL_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/math/abs.h"
#include "castle/math/minmax.h"

namespace castle
{
namespace math
{

// ============================================================================
// Absolute/relative floating-point comparison.
//
// epsilon is the caller's tolerance and is deliberately explicit. Geometry
// algorithms often need a tolerance matched to coordinate units and sensor
// noise rather than a universal magic constant.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, bool>::type
near_equal(T a, T b, T epsilon) CASTLE_NOEXCEPT
{
    CASTLE_CONST T difference = abs(static_cast<T>(a - b));
    CASTLE_CONST T scale = max(static_cast<T>(1),
                        max(abs(a), abs(b)));
    return difference <= epsilon * scale;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_NEAR_EQUAL_H
