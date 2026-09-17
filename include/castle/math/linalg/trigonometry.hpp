// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file
 * @brief Floating-point trigonometric wrappers for Castle linear algebra.
 *
 * This header centralizes Castle's trigonometric entry points behind a small
 * wrapper layer around the C math library. Use it when fixed-size matrix,
 * quaternion, or transform code needs sine and cosine without introducing any
 * STL dependency. The functions are allocation-free and exception-free; their
 * precision and determinism are those of the platform's `sinf`, `sin`,
 * `sinl`, `cosf`, `cos`, and `cosl` implementations rather than custom Castle
 * approximations.
 *
 * @code
 * #include "castle/math/linalg/trigonometry.hpp"
 * #include "castle/core/constants.hpp"
 *
 * const castle::math::sin_cos_result<float> values =
 *     castle::math::sin_cos(castle::math::pi<float>() * 0.5F);
 * @endcode
 */
#ifndef CASTLE_MATH_TRIGONOMETRY_HPP
#define CASTLE_MATH_TRIGONOMETRY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

#include <math.h>

namespace castle
{
namespace math
{

/**
 * @brief Returns the sine of a radian angle as `float`.
 * @param radians Input angle in radians.
 * @return `sinf(radians)`.
 * @note Precision and range reduction come from the platform C math library.
 */
inline float sin(float radians) CASTLE_NOEXCEPT
{
    return ::sinf(radians);
}

/**
 * @brief Returns the sine of a radian angle as `double`.
 * @param radians Input angle in radians.
 * @return `sin(radians)`.
 * @note Precision and range reduction come from the platform C math library.
 */
inline double sin(double radians) CASTLE_NOEXCEPT
{
    return ::sin(radians);
}

/**
 * @brief Returns the sine of a radian angle as `long double`.
 * @param radians Input angle in radians.
 * @return `sinl(radians)`.
 * @note Precision and range reduction come from the platform C math library.
 */
inline long double sin(long double radians) CASTLE_NOEXCEPT
{
    return ::sinl(radians);
}

/**
 * @brief Returns the cosine of a radian angle as `float`.
 * @param radians Input angle in radians.
 * @return `cosf(radians)`.
 * @note Precision and range reduction come from the platform C math library.
 */
inline float cos(float radians) CASTLE_NOEXCEPT
{
    return ::cosf(radians);
}

/**
 * @brief Returns the cosine of a radian angle as `double`.
 * @param radians Input angle in radians.
 * @return `cos(radians)`.
 * @note Precision and range reduction come from the platform C math library.
 */
inline double cos(double radians) CASTLE_NOEXCEPT
{
    return ::cos(radians);
}

/**
 * @brief Returns the cosine of a radian angle as `long double`.
 * @param radians Input angle in radians.
 * @return `cosl(radians)`.
 * @note Precision and range reduction come from the platform C math library.
 */
inline long double cos(long double radians) CASTLE_NOEXCEPT
{
    return ::cosl(radians);
}

/**
 * @brief Returns the sine of a radian angle for a floating-point type.
 * @tparam T Floating-point angle type.
 * @param radians Input angle in radians.
 * @return The sine of `radians`.
 * @warning This template is restricted to floating-point types.
 */
template <typename T>
CASTLE_NODISCARD T radians_sin(T radians) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "radians_sin requires a floating-point type");
    return castle::math::sin(radians);
}

/**
 * @brief Returns the cosine of a radian angle for a floating-point type.
 * @tparam T Floating-point angle type.
 * @param radians Input angle in radians.
 * @return The cosine of `radians`.
 * @warning This template is restricted to floating-point types.
 */
template <typename T>
CASTLE_NODISCARD T radians_cos(T radians) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "radians_cos requires a floating-point type");
    return castle::math::cos(radians);
}

/**
 * @brief Bundles sine and cosine values computed for the same angle.
 * @tparam T Floating-point result type.
 * @note The fields are public aggregate members with no hidden storage.
 */
template <typename T>
struct sin_cos_result
{
    /** @brief Sine of the input angle. */
    T sine;
    /** @brief Cosine of the input angle. */
    T cosine;
};

/**
 * @brief Computes sine and cosine for one radian angle.
 * @tparam T Floating-point angle type.
 * @param radians Input angle in radians.
 * @return Aggregate containing both the sine and cosine.
 * @warning This template is restricted to floating-point types.
 */
template <typename T>
CASTLE_NODISCARD sin_cos_result<T> sin_cos(T radians) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "sin_cos requires a floating-point type");
    return sin_cos_result<T>{castle::math::sin(radians), castle::math::cos(radians)};
}

} // namespace math
} // namespace castle

#endif
