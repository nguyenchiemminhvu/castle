// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Internal helper utilities shared by Castle geometry headers.
 *
 * This header collects tiny constexpr building blocks used by higher-level
 * geometry primitives and algorithms. The helpers stay heap-free and favor
 * widened integer intermediates for common 2D and 3D calculations.
 */
#ifndef CASTLE_MATH_GEOMETRY_DETAIL_HPP
#define CASTLE_MATH_GEOMETRY_DETAIL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/sqrt_real.hpp"

namespace castle
{
namespace math
{
namespace detail
{

/**
 * @brief Selects an intermediate calculation type for geometry formulas.
 * @tparam T Coordinate type.
 * @note Integral types no wider than int32_t are promoted to int64_t to reduce
 * intermediate overflow in determinant and area calculations.
 */
template <typename T>
using geometry_calc_type = typename meta::conditional<
    meta::is_integral<T>::value && (sizeof(T) <= sizeof(int32_t)),
    int64_t,
    T>::type;

/**
 * @brief Computes the signed 2D cross-product determinant.
 * @tparam T Arithmetic type.
 * @param ax X component of the first vector.
 * @param ay Y component of the first vector.
 * @param bx X component of the second vector.
 * @param by Y component of the second vector.
 * @return ax * by - ay * bx.
 * @note This is the z component of the 3D cross product for vectors in the XY plane.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross2(T ax, T ay, T bx, T by) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * by - ay * bx);
}

/**
 * @brief Computes the 2D dot product.
 * @tparam T Arithmetic type.
 * @param ax X component of the first vector.
 * @param ay Y component of the first vector.
 * @param bx X component of the second vector.
 * @param by Y component of the second vector.
 * @return ax * bx + ay * by.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot2(T ax, T ay, T bx, T by) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * bx + ay * by);
}

/**
 * @brief Computes the 3D dot product.
 * @tparam T Arithmetic type.
 * @param ax X component of the first vector.
 * @param ay Y component of the first vector.
 * @param az Z component of the first vector.
 * @param bx X component of the second vector.
 * @param by Y component of the second vector.
 * @param bz Z component of the second vector.
 * @return ax * bx + ay * by + az * bz.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot3(
    T ax, T ay, T az,
    T bx, T by, T bz) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * bx + ay * by + az * bz);
}

/**
 * @brief Computes the x component of a 3D cross product.
 * @tparam T Arithmetic type.
 * @param ay Y component of the first vector.
 * @param az Z component of the first vector.
 * @param by Y component of the second vector.
 * @param bz Z component of the second vector.
 * @return ay * bz - az * by.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross3_x(
    T ay, T az, T by, T bz) CASTLE_NOEXCEPT
{
    return static_cast<T>(ay * bz - az * by);
}

/**
 * @brief Computes the y component of a 3D cross product.
 * @tparam T Arithmetic type.
 * @param az Z component of the first vector.
 * @param ax X component of the first vector.
 * @param bz Z component of the second vector.
 * @param bx X component of the second vector.
 * @return az * bx - ax * bz.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross3_y(
    T az, T ax, T bz, T bx) CASTLE_NOEXCEPT
{
    return static_cast<T>(az * bx - ax * bz);
}

/**
 * @brief Computes the z component of a 3D cross product.
 * @tparam T Arithmetic type.
 * @param ax X component of the first vector.
 * @param ay Y component of the first vector.
 * @param bx X component of the second vector.
 * @param by Y component of the second vector.
 * @return ax * by - ay * bx.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross3_z(
    T ax, T ay, T bx, T by) CASTLE_NOEXCEPT
{
    return static_cast<T>(ax * by - ay * bx);
}

/**
 * @brief Tests whether a value is within an absolute tolerance of zero.
 * @tparam T Arithmetic type.
 * @param value Value to test.
 * @param epsilon Non-negative absolute tolerance.
 * @return true when abs(value) <= epsilon.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool near_zero(T value, T epsilon) CASTLE_NOEXCEPT
{
    return abs(value) <= epsilon;
}

}
}
}

#endif
