#ifndef CASTLE_MATH_MINMAX_H
#define CASTLE_MATH_MINMAX_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ----------------------------------------------------------------------------
// Deterministic scalar min/max helpers.
//
// Real-life use cases:
//   - Bounding sensor values before conversion to actuator commands.
//   - Selecting a safe operating limit without pulling in the STL.
// ----------------------------------------------------------------------------

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T min(T a, T b) CASTLE_NOEXCEPT
{
    return (b < a) ? b : a;
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T max(T a, T b) CASTLE_NOEXCEPT
{
    return (a < b) ? b : a;
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T min3(T a, T b, T c) CASTLE_NOEXCEPT
{
    return min(min(a, b), c);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T max3(T a, T b, T c) CASTLE_NOEXCEPT
{
    return max(max(a, b), c);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_MINMAX_H
