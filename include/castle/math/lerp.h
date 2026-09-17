#ifndef CASTLE_MATH_LERP_H
#define CASTLE_MATH_LERP_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// Linear interpolation.
//
// lerp(a, b, t) returns a + t * (b - a). No clamping is performed.
// Floating point is intentional: an integer interpolation parameter would
// silently lose the fractional part and is usually a caller bug.
//
// Real-life use cases:
//   - Sensor calibration curves.
//   - Smooth set-point transitions.
//   - Lookup-table interpolation.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
lerp(T a, T b, T t) CASTLE_NOEXCEPT
{
    return a + t * (b - a);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_LERP_H
