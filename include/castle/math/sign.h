#ifndef CASTLE_MATH_SIGN_H
#define CASTLE_MATH_SIGN_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ----------------------------------------------------------------------------
// Sign helper.
//
// Returns -1 for negative values, 0 for zero, and +1 for positive values.
// For unsigned types the result is 0 or +1.
//
// Real-life use cases:
//   - Determining motion direction from an encoder error.
//   - Fast branch decisions in steering and control algorithms.
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
int sign(T value) CASTLE_NOEXCEPT
{
    return (value > T{0}) ? 1 : ((value < T{0}) ? -1 : 0);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_SIGN_H
