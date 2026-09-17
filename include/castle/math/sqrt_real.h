#ifndef CASTLE_MATH_SQRT_REAL_H
#define CASTLE_MATH_SQRT_REAL_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// Runtime/constexpr floating-point square root.
//
// The existing castle::sqrt<Value> template provides an integer floor square
// root. This function covers the floating-point case used by geometry and
// control calculations without depending on <math.h>.
//
// Newton-Raphson is used with a deterministic iteration bound. The input is
// range-reduced first so very small or very large finite values converge with
// the same bounded number of iterations.
//
// Real-life use cases:
//   - Euclidean distance in sensor coordinates.
//   - RMS and norm calculations in control loops.
//   - Circle/line intersection calculations.
// ============================================================================

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
sqrt_real(T value) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(value >= static_cast<T>(0),
                  CASTLE_ERROR_GENERIC("castle::math::sqrt_real: negative input"));

    if (value == static_cast<T>(0))
    {
        return static_cast<T>(0);
    }

    // Keep the Newton iteration close to unity. Every scaling step changes
    // the square-root magnitude by a factor of two.
    T scaled = value;
    T scale = static_cast<T>(1);
    const unsigned scale_steps = (sizeof(T) <= sizeof(float)) ? 64U : 512U;

    for (unsigned i = 0U; i < scale_steps; ++i)
    {
        if (scaled > static_cast<T>(4))
        {
            scaled *= static_cast<T>(0.25);
            scale *= static_cast<T>(2);
        }
    }

    for (unsigned i = 0U; i < scale_steps; ++i)
    {
        if (scaled < static_cast<T>(0.25))
        {
            scaled *= static_cast<T>(4);
            scale *= static_cast<T>(0.5);
        }
    }

    T guess = static_cast<T>(1);
    const unsigned newton_steps = (sizeof(T) <= sizeof(float)) ? 24U : 32U;

    for (unsigned i = 0U; i < newton_steps; ++i)
    {
        const T next = static_cast<T>(0.5) *
                       (guess + scaled / guess);
        if (next == guess)
        {
            break;
        }
        guess = next;
    }

    return guess * scale;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_SQRT_REAL_H
