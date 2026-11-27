// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sqrt_real.hpp
 * @brief Deterministic floating-point square root helper.
 *
 * This header computes the square root of a non-negative floating-point value
 * without requiring callers to depend directly on `<math.h>`. Use it for
 * geometry, RMS, norm, and control calculations that need a reproducible,
 * heap-free implementation.
 *
 * Example:
 * @code
 * const float distance = castle::math::sqrt_real(2.25f);
 * const double root_two = castle::math::sqrt_real(2.0);
 * (void)distance;
 * (void)root_two;
 * @endcode
 *
 * @note GCC/Clang builds use builtin square-root operations when available;
 *       other builds use deterministic range reduction plus Newton iteration.
 */
#ifndef CASTLE_MATH_SQRT_REAL_HPP
#define CASTLE_MATH_SQRT_REAL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes the square root of a non-negative floating-point value.
 * @tparam T Floating-point type.
 * @param value Input value.
 * @return Square root of `value`.
 * @note The fallback implementation uses a fixed iteration budget after
 *       scaling the input toward unity for stable convergence.
 * @warning Negative inputs violate the precondition and trigger assertion
 *          handling before continuing through the configured build mode.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
sqrt_real(T value) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(value >= static_cast<T>(0), // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::sqrt_real: negative input"));

    if (value == static_cast<T>(0))
    {
        return static_cast<T>(0);
    }

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

    // Scale toward [0.25, 4] so the bounded Newton iteration stays well-conditioned.
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
