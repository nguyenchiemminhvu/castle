// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file exponential.hpp
 * @brief Natural exponential and logarithm wrappers for floating-point types.
 * @details Mirrors the layout of the trigonometric wrappers in
 * `castle/math/linalg/trigonometry.hpp`: thin, allocation-free overloads over
 * the platform C math library (`expf`/`exp`/`expl`, `logf`/`log`/`logl`).
 * Precision and determinism are those of the platform implementation. They
 * are the scalar building blocks used by `castle::math::complex`.
 *
 * @code
 * #include "castle/math/exponential.hpp"
 *
 * const double e1 = castle::math::exp(1.0);
 * const double one = castle::math::log(e1);
 * @endcode
 */
#ifndef CASTLE_MATH_EXPONENTIAL_HPP
#define CASTLE_MATH_EXPONENTIAL_HPP

#include "castle/core/compiler.hpp"

#include <math.h>

namespace castle
{
namespace math
{

/**
 * @brief Returns `e` raised to `value` as `float`.
 * @param value Exponent.
 * @return `expf(value)`.
 */
CASTLE_INLINE float exp(float value) CASTLE_NOEXCEPT
{
    return ::expf(value);
}

/**
 * @brief Returns `e` raised to `value` as `double`.
 * @param value Exponent.
 * @return `exp(value)`.
 */
CASTLE_INLINE double exp(double value) CASTLE_NOEXCEPT
{
    return ::exp(value);
}

/**
 * @brief Returns `e` raised to `value` as `long double`.
 * @param value Exponent.
 * @return `expl(value)`.
 */
CASTLE_INLINE long double exp(long double value) CASTLE_NOEXCEPT
{
    return ::expl(value);
}

/**
 * @brief Returns the natural logarithm of a `float`.
 * @param value Strictly positive input.
 * @return `logf(value)`.
 * @warning Zero yields `-inf` and negative values yield NaN, as in the C library.
 */
CASTLE_INLINE float log(float value) CASTLE_NOEXCEPT
{
    return ::logf(value);
}

/**
 * @brief Returns the natural logarithm of a `double`.
 * @param value Strictly positive input.
 * @return `log(value)`.
 * @warning Zero yields `-inf` and negative values yield NaN, as in the C library.
 */
CASTLE_INLINE double log(double value) CASTLE_NOEXCEPT
{
    return ::log(value);
}

/**
 * @brief Returns the natural logarithm of a `long double`.
 * @param value Strictly positive input.
 * @return `logl(value)`.
 * @warning Zero yields `-inf` and negative values yield NaN, as in the C library.
 */
CASTLE_INLINE long double log(long double value) CASTLE_NOEXCEPT
{
    return ::logl(value);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_EXPONENTIAL_HPP
