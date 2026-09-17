// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file
 * @brief Homogeneous 2D and 3D transform helpers for Castle linear algebra.
 *
 * This header provides fixed-size affine transform builders and application
 * helpers for vectors, points, and geometry types. Use it when you need
 * deterministic transform composition with no dynamic allocation, exceptions,
 * RTTI, virtual dispatch, or STL containers. Matrices are stored row-major,
 * vectors are treated as columns, translations live in the final column, and
 * composition order follows the matrix product exactly: `A * B` applies `B`
 * first and then `A`. Angle-dependent helpers rely on the floating-point
 * wrappers in `trigonometry.hpp`.
 *
 * @code
 * #include "castle/math/linalg/transform.hpp"
 * #include "castle/core/constants.hpp"
 *
 * const castle::math::matrix<float, 3U, 3U> transform =
 *     castle::math::compose_transform_2d(
 *         castle::math::vector<float, 2U>(2.0F, 3.0F),
 *         castle::math::degrees_to_radians(90.0F),
 *         castle::math::vector<float, 2U>(2.0F, 1.0F));
 * @endcode
 */
#ifndef CASTLE_MATH_TRANSFORM_HPP
#define CASTLE_MATH_TRANSFORM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/constants.hpp"
#include "castle/math/geometry/point2d.hpp"
#include "castle/math/geometry/point3d.hpp"
#include "castle/math/geometry/vector2d.hpp"
#include "castle/math/geometry/vector3d.hpp"
#include "castle/math/linalg/matrix.hpp"
#include "castle/math/linalg/trigonometry.hpp"
#include "castle/math/linalg/vector.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Builds a 2D homogeneous translation matrix.
 * @tparam T Matrix element type.
 * @param x Translation along the X axis.
 * @param y Translation along the Y axis.
 * @return Row-major 3x3 matrix that translates column vectors by `(x, y)`.
 * @note Translation is stored in the final matrix column.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> translation_2d(T x, T y) CASTLE_NOEXCEPT
{
    matrix<T, 3U, 3U> result = identity<T, 3U>();
    result(0U, 2U) = x;
    result(1U, 2U) = y;
    return result;
}

/**
 * @brief Builds a 2D homogeneous translation matrix from a vector.
 * @tparam T Matrix element type.
 * @param value Translation vector `(x, y)`.
 * @return Row-major 3x3 translation matrix.
 * @note Translation is stored in the final matrix column.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> translation_2d(CASTLE_CONST vector<T, 2U>& value) CASTLE_NOEXCEPT
{
    return translation_2d(value[0U], value[1U]);
}

/**
 * @brief Builds a 2D homogeneous scaling matrix.
 * @tparam T Matrix element type.
 * @param x Scale factor along the X axis.
 * @param y Scale factor along the Y axis.
 * @return Row-major 3x3 matrix that scales column vectors by `(x, y)`.
 * @note Off-diagonal entries remain zero and the homogeneous term is one.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> scaling_2d(T x, T y) CASTLE_NOEXCEPT
{
    matrix<T, 3U, 3U> result;
    result(0U, 0U) = x;
    result(1U, 1U) = y;
    result(2U, 2U) = static_cast<T>(1);
    return result;
}

/**
 * @brief Builds a 2D homogeneous scaling matrix from a vector.
 * @tparam T Matrix element type.
 * @param value Scale vector `(x, y)`.
 * @return Row-major 3x3 scaling matrix.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> scaling_2d(CASTLE_CONST vector<T, 2U>& value) CASTLE_NOEXCEPT
{
    return scaling_2d(value[0U], value[1U]);
}

/**
 * @brief Builds a 2D rotation matrix from a radian angle.
 * @tparam T Floating-point angle and matrix element type.
 * @param radians Counterclockwise rotation angle in radians.
 * @return Row-major 3x3 matrix that rotates column vectors in the XY plane.
 * @note The matrix is compatible with `transform_point_2d()` and `transform_vector_2d()`.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> rotation_2d(T radians) CASTLE_NOEXCEPT
{
    CASTLE_CONST sin_cos_result<T> values = sin_cos(radians);
    matrix<T, 3U, 3U> result = identity<T, 3U>();
    result(0U, 0U) = values.cosine;
    result(0U, 1U) = static_cast<T>(-values.sine);
    result(1U, 0U) = values.sine;
    result(1U, 1U) = values.cosine;
    return result;
}

/**
 * @brief Builds a 2D rotation matrix from a degree angle.
 * @tparam T Floating-point angle and matrix element type.
 * @param degrees Counterclockwise rotation angle in degrees.
 * @return Row-major 3x3 matrix that rotates column vectors in the XY plane.
 * @note Internally converts degrees to radians before calling `rotation_2d()`.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> rotation_2d_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_2d(degrees_to_radians(degrees));
}

/**
 * @brief Applies a 3x3 homogeneous transform to a 2D point vector.
 * @tparam T Matrix and vector element type.
 * @param transform Row-major 3x3 transform matrix.
 * @param point Point expressed as a 2D vector.
 * @return Transformed 2D point vector.
 * @note The input uses homogeneous coordinate `w = 1`, so translation affects the result.
 * @warning Triggers Castle's assertion/error path when the resulting homogeneous coordinate is zero.
 */
template <typename T>
CASTLE_NODISCARD vector<T, 2U> transform_point_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST vector<T, 2U>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> homogeneous(point[0U], point[1U], static_cast<T>(1));
    CASTLE_CONST vector<T, 3U> result = transform * homogeneous;
    CASTLE_ASSERT(result[2U] != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::transform_point_2d: invalid homogeneous coordinate"));

    if (result[2U] == static_cast<T>(1)) // LCOV_EXCL_BR_LINE
    {
        return vector<T, 2U>(result[0U], result[1U]);
    }

    // LCOV_EXCL_START
    return vector<T, 2U>(
        static_cast<T>(result[0U] / result[2U]),
        static_cast<T>(result[1U] / result[2U]));
    // LCOV_EXCL_STOP
}

/**
 * @brief Applies a 3x3 homogeneous transform to a 2D direction vector.
 * @tparam T Matrix and vector element type.
 * @param transform Row-major 3x3 transform matrix.
 * @param value Direction expressed as a 2D vector.
 * @return Transformed 2D direction vector.
 * @note The input uses homogeneous coordinate `w = 0`, so translation is ignored.
 */
template <typename T>
CASTLE_NODISCARD vector<T, 2U> transform_vector_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST vector<T, 2U>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> homogeneous(value[0U], value[1U], static_cast<T>(0));
    CASTLE_CONST vector<T, 3U> result = transform * homogeneous;
    return vector<T, 2U>(result[0U], result[1U]);
}

/**
 * @brief Applies a 3x3 homogeneous transform to a `point2d`.
 * @tparam T Matrix and point element type.
 * @param transform Row-major 3x3 transform matrix.
 * @param point Input point.
 * @return Transformed point.
 * @note This overload converts through `vector<T, 2U>`.
 * @warning Triggers Castle's assertion/error path when the resulting homogeneous coordinate is zero.
 */
template <typename T>
CASTLE_NODISCARD point2d<T> transform_point_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST point2d<T>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 2U> result = transform_point_2d(
        transform, vector<T, 2U>(point.x(), point.y()));
    return point2d<T>(result[0U], result[1U]);
}

/**
 * @brief Applies a 3x3 homogeneous transform to a `vector2d`.
 * @tparam T Matrix and vector element type.
 * @param transform Row-major 3x3 transform matrix.
 * @param value Input direction vector.
 * @return Transformed direction vector.
 * @note This overload converts through `vector<T, 2U>`.
 */
template <typename T>
CASTLE_NODISCARD vector2d<T> transform_vector_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 2U> result = transform_vector_2d(
        transform, vector<T, 2U>(value.x(), value.y()));
    return vector2d<T>(result[0U], result[1U]);
}

/**
 * @brief Builds a 3D homogeneous translation matrix.
 * @tparam T Matrix element type.
 * @param x Translation along the X axis.
 * @param y Translation along the Y axis.
 * @param z Translation along the Z axis.
 * @return Row-major 4x4 matrix that translates column vectors by `(x, y, z)`.
 * @note Translation is stored in the final matrix column.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> translation_3d(T x, T y, T z) CASTLE_NOEXCEPT
{
    matrix<T, 4U, 4U> result = identity<T, 4U>();
    result(0U, 3U) = x;
    result(1U, 3U) = y;
    result(2U, 3U) = z;
    return result;
}

/**
 * @brief Builds a 3D homogeneous translation matrix from a vector.
 * @tparam T Matrix element type.
 * @param value Translation vector `(x, y, z)`.
 * @return Row-major 4x4 translation matrix.
 * @note Translation is stored in the final matrix column.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> translation_3d(CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    return translation_3d(value[0U], value[1U], value[2U]);
}

/**
 * @brief Builds a 3D homogeneous scaling matrix.
 * @tparam T Matrix element type.
 * @param x Scale factor along the X axis.
 * @param y Scale factor along the Y axis.
 * @param z Scale factor along the Z axis.
 * @return Row-major 4x4 matrix that scales column vectors by `(x, y, z)`.
 * @note Off-diagonal entries remain zero and the homogeneous term is one.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> scaling_3d(T x, T y, T z) CASTLE_NOEXCEPT
{
    matrix<T, 4U, 4U> result;
    result(0U, 0U) = x;
    result(1U, 1U) = y;
    result(2U, 2U) = z;
    result(3U, 3U) = static_cast<T>(1);
    return result;
}

/**
 * @brief Builds a 3D homogeneous scaling matrix from a vector.
 * @tparam T Matrix element type.
 * @param value Scale vector `(x, y, z)`.
 * @return Row-major 4x4 scaling matrix.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> scaling_3d(CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    return scaling_3d(value[0U], value[1U], value[2U]);
}

/**
 * @brief Builds a 3D rotation matrix about the X axis.
 * @tparam T Floating-point angle and matrix element type.
 * @param radians Rotation angle in radians.
 * @return Row-major 4x4 matrix that rotates column vectors about +X.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_x(T radians) CASTLE_NOEXCEPT
{
    CASTLE_CONST sin_cos_result<T> values = sin_cos(radians);
    matrix<T, 4U, 4U> result = identity<T, 4U>();
    result(1U, 1U) = values.cosine;
    result(1U, 2U) = static_cast<T>(-values.sine);
    result(2U, 1U) = values.sine;
    result(2U, 2U) = values.cosine;
    return result;
}

/**
 * @brief Builds a 3D rotation matrix about the Y axis.
 * @tparam T Floating-point angle and matrix element type.
 * @param radians Rotation angle in radians.
 * @return Row-major 4x4 matrix that rotates column vectors about +Y.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_y(T radians) CASTLE_NOEXCEPT
{
    CASTLE_CONST sin_cos_result<T> values = sin_cos(radians);
    matrix<T, 4U, 4U> result = identity<T, 4U>();
    result(0U, 0U) = values.cosine;
    result(0U, 2U) = values.sine;
    result(2U, 0U) = static_cast<T>(-values.sine);
    result(2U, 2U) = values.cosine;
    return result;
}

/**
 * @brief Builds a 3D rotation matrix about the Z axis.
 * @tparam T Floating-point angle and matrix element type.
 * @param radians Rotation angle in radians.
 * @return Row-major 4x4 matrix that rotates column vectors about +Z.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_z(T radians) CASTLE_NOEXCEPT
{
    CASTLE_CONST sin_cos_result<T> values = sin_cos(radians);
    matrix<T, 4U, 4U> result = identity<T, 4U>();
    result(0U, 0U) = values.cosine;
    result(0U, 1U) = static_cast<T>(-values.sine);
    result(1U, 0U) = values.sine;
    result(1U, 1U) = values.cosine;
    return result;
}

/**
 * @brief Builds a 3D rotation matrix about the X axis from degrees.
 * @tparam T Floating-point angle and matrix element type.
 * @param degrees Rotation angle in degrees.
 * @return Row-major 4x4 matrix that rotates column vectors about +X.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_x_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_x(degrees_to_radians(degrees));
}

/**
 * @brief Builds a 3D rotation matrix about the Y axis from degrees.
 * @tparam T Floating-point angle and matrix element type.
 * @param degrees Rotation angle in degrees.
 * @return Row-major 4x4 matrix that rotates column vectors about +Y.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_y_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_y(degrees_to_radians(degrees));
}

/**
 * @brief Builds a 3D rotation matrix about the Z axis from degrees.
 * @tparam T Floating-point angle and matrix element type.
 * @param degrees Rotation angle in degrees.
 * @return Row-major 4x4 matrix that rotates column vectors about +Z.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_z_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_z(degrees_to_radians(degrees));
}

/**
 * @brief Builds a 3x3 axis-angle rotation matrix.
 * @tparam T Floating-point angle and matrix element type.
 * @param axis Rotation axis.
 * @param radians Rotation angle in radians.
 * @return Row-major 3x3 matrix that rotates column vectors about `axis`.
 * @note The axis is normalized before constructing the matrix.
 * @warning Triggers Castle's assertion/error path when `axis` is the zero vector.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> rotation_axis_angle_3d(
    CASTLE_CONST vector<T, 3U>& axis,
    T radians) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> unit_axis = axis.normalized();
    CASTLE_CONST sin_cos_result<T> values = sin_cos(radians);
    CASTLE_CONST T c = values.cosine;
    CASTLE_CONST T s = values.sine;
    CASTLE_CONST T t = static_cast<T>(1 - c);
    CASTLE_CONST T x = unit_axis[0U];
    CASTLE_CONST T y = unit_axis[1U];
    CASTLE_CONST T z = unit_axis[2U];

    return matrix<T, 3U, 3U>(
        static_cast<T>(t * x * x + c),
        static_cast<T>(t * x * y - s * z),
        static_cast<T>(t * x * z + s * y),
        static_cast<T>(t * x * y + s * z),
        static_cast<T>(t * y * y + c),
        static_cast<T>(t * y * z - s * x),
        static_cast<T>(t * x * z - s * y),
        static_cast<T>(t * y * z + s * x),
        static_cast<T>(t * z * z + c));
}

/**
 * @brief Builds a 4x4 homogeneous axis-angle rotation matrix.
 * @tparam T Floating-point angle and matrix element type.
 * @param axis Rotation axis.
 * @param radians Rotation angle in radians.
 * @return Row-major 4x4 matrix that rotates column vectors about `axis`.
 * @note The upper-left 3x3 block comes from `rotation_axis_angle_3d()`.
 * @warning Triggers Castle's assertion/error path when `axis` is the zero vector.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_axis_angle(
    CASTLE_CONST vector<T, 3U>& axis,
    T radians) CASTLE_NOEXCEPT
{
    CASTLE_CONST matrix<T, 3U, 3U> rotation = rotation_axis_angle_3d(axis, radians);
    matrix<T, 4U, 4U> result = identity<T, 4U>();
    for (size_type row = 0U; row < 3U; ++row)
    {
        for (size_type column = 0U; column < 3U; ++column)
        {
            result(row, column) = rotation(row, column);
        }
    }
    return result;
}

/**
 * @brief Builds a 4x4 homogeneous axis-angle rotation matrix from degrees.
 * @tparam T Floating-point angle and matrix element type.
 * @param axis Rotation axis.
 * @param degrees Rotation angle in degrees.
 * @return Row-major 4x4 matrix that rotates column vectors about `axis`.
 * @warning Triggers Castle's assertion/error path when `axis` is the zero vector.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_axis_angle_degrees(
    CASTLE_CONST vector<T, 3U>& axis,
    T degrees) CASTLE_NOEXCEPT
{
    return rotation_axis_angle(axis, degrees_to_radians(degrees));
}

/**
 * @brief Composes XYZ Euler rotations into one homogeneous matrix.
 * @tparam T Floating-point angle and matrix element type.
 * @param x_radians Rotation about the X axis.
 * @param y_radians Rotation about the Y axis.
 * @param z_radians Rotation about the Z axis.
 * @return Row-major 4x4 matrix equal to `rotation_z(z) * rotation_y(y) * rotation_x(x)`.
 * @note When applied to a column vector, the result performs X, then Y, then Z rotation.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_xyz(
    T x_radians,
    T y_radians,
    T z_radians) CASTLE_NOEXCEPT
{
    return rotation_z(z_radians) * rotation_y(y_radians) * rotation_x(x_radians);
}

/**
 * @brief Composes a 2D transform from translation, rotation, and scale.
 * @tparam T Floating-point angle and matrix element type.
 * @param translation Translation vector.
 * @param rotation_radians Rotation angle in radians.
 * @param scale Scale vector.
 * @return Row-major 3x3 matrix equal to `T * R * S`.
 * @note When applied to a column vector, the result performs scale first, then rotation, then translation.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> compose_transform_2d(
    CASTLE_CONST vector<T, 2U>& translation,
    T rotation_radians,
    CASTLE_CONST vector<T, 2U>& scale) CASTLE_NOEXCEPT
{
    return translation_2d(translation) * rotation_2d(rotation_radians) * scaling_2d(scale);
}

/**
 * @brief Composes a 3D transform from translation, XYZ rotation, and scale.
 * @tparam T Floating-point angle and matrix element type.
 * @param translation Translation vector.
 * @param x_rotation_radians Rotation about the X axis.
 * @param y_rotation_radians Rotation about the Y axis.
 * @param z_rotation_radians Rotation about the Z axis.
 * @param scale Scale vector.
 * @return Row-major 4x4 matrix equal to `T * rotation_xyz(x, y, z) * S`.
 * @note When applied to a column vector, the result performs scale first, then X/Y/Z rotation, then translation.
 */
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> compose_transform_3d(
    CASTLE_CONST vector<T, 3U>& translation,
    T x_rotation_radians,
    T y_rotation_radians,
    T z_rotation_radians,
    CASTLE_CONST vector<T, 3U>& scale) CASTLE_NOEXCEPT
{
    return translation_3d(translation) *
           rotation_xyz(x_rotation_radians, y_rotation_radians, z_rotation_radians) *
           scaling_3d(scale);
}

/**
 * @brief Applies a 4x4 homogeneous transform to a 3D point vector.
 * @tparam T Matrix and vector element type.
 * @param transform Row-major 4x4 transform matrix.
 * @param point Point expressed as a 3D vector.
 * @return Transformed 3D point vector.
 * @note The input uses homogeneous coordinate `w = 1`, so translation affects the result.
 * @warning Triggers Castle's assertion/error path when the resulting homogeneous coordinate is zero.
 */
template <typename T>
CASTLE_NODISCARD vector<T, 3U> transform_point_3d(
    CASTLE_CONST matrix<T, 4U, 4U>& transform,
    CASTLE_CONST vector<T, 3U>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 4U> homogeneous(point[0U], point[1U], point[2U], static_cast<T>(1));
    CASTLE_CONST vector<T, 4U> result = transform * homogeneous;
    CASTLE_ASSERT(result[3U] != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::transform_point_3d: invalid homogeneous coordinate"));

    if (result[3U] == static_cast<T>(1)) // LCOV_EXCL_BR_LINE
    {
        return vector<T, 3U>(result[0U], result[1U], result[2U]);
    }

    // LCOV_EXCL_START
    return vector<T, 3U>(
        static_cast<T>(result[0U] / result[3U]),
        static_cast<T>(result[1U] / result[3U]),
        static_cast<T>(result[2U] / result[3U]));
    // LCOV_EXCL_STOP
}

/**
 * @brief Applies a 4x4 homogeneous transform to a 3D direction vector.
 * @tparam T Matrix and vector element type.
 * @param transform Row-major 4x4 transform matrix.
 * @param value Direction expressed as a 3D vector.
 * @return Transformed 3D direction vector.
 * @note The input uses homogeneous coordinate `w = 0`, so translation is ignored.
 */
template <typename T>
CASTLE_NODISCARD vector<T, 3U> transform_vector_3d(
    CASTLE_CONST matrix<T, 4U, 4U>& transform,
    CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 4U> homogeneous(value[0U], value[1U], value[2U], static_cast<T>(0));
    CASTLE_CONST vector<T, 4U> result = transform * homogeneous;
    return vector<T, 3U>(result[0U], result[1U], result[2U]);
}

/**
 * @brief Applies a 4x4 homogeneous transform to a `point3d`.
 * @tparam T Matrix and point element type.
 * @param transform Row-major 4x4 transform matrix.
 * @param point Input point.
 * @return Transformed point.
 * @note This overload converts through `vector<T, 3U>`.
 * @warning Triggers Castle's assertion/error path when the resulting homogeneous coordinate is zero.
 */
template <typename T>
CASTLE_NODISCARD point3d<T> transform_point_3d(
    CASTLE_CONST matrix<T, 4U, 4U>& transform,
    CASTLE_CONST point3d<T>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> result = transform_point_3d(
        transform, vector<T, 3U>(point.x(), point.y(), point.z()));
    return point3d<T>(result[0U], result[1U], result[2U]);
}

/**
 * @brief Applies a 4x4 homogeneous transform to a `vector3d`.
 * @tparam T Matrix and vector element type.
 * @param transform Row-major 4x4 transform matrix.
 * @param value Input direction vector.
 * @return Transformed direction vector.
 * @note This overload converts through `vector<T, 3U>`.
 */
template <typename T>
CASTLE_NODISCARD vector3d<T> transform_vector_3d(
    CASTLE_CONST matrix<T, 4U, 4U>& transform,
    CASTLE_CONST vector3d<T>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> result = transform_vector_3d(
        transform, vector<T, 3U>(value.x(), value.y(), value.z()));
    return vector3d<T>(result[0U], result[1U], result[2U]);
}

} // namespace math
} // namespace castle

#endif
