#ifndef CASTLE_MATH_SQRT_REAL_HPP
#define CASTLE_MATH_SQRT_REAL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

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

    // GCC/Clang (including armclang) fold __builtin_sqrt into a single
    // hardware sqrt instruction at runtime, and can still evaluate it at
    // compile time inside a constexpr context, so this keeps both usages
    // fast without giving up determinism (IEEE-754 sqrt is exactly rounded).
#if defined(__GNUC__) || defined(__clang__)
    if (meta::is_same<T, float>::value)
    {
        return static_cast<T>(__builtin_sqrtf(static_cast<float>(value)));
    }
    if (meta::is_same<T, double>::value)
    {
        return static_cast<T>(__builtin_sqrt(static_cast<double>(value)));
    }
#endif

    // Keep the Newton iteration close to unity. Every scaling step changes
    // the square-root magnitude by a factor of two.
    T scaled = value;
    T scale = static_cast<T>(1);
    CASTLE_CONST unsigned scale_steps = (sizeof(T) <= sizeof(float)) ? 64U : 512U;

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
    CASTLE_CONST unsigned newton_steps = (sizeof(T) <= sizeof(float)) ? 24U : 32U;

    for (unsigned i = 0U; i < newton_steps; ++i)
    {
        CASTLE_CONST T next = static_cast<T>(0.5) *
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

#endif // CASTLE_MATH_SQRT_REAL_HPP
