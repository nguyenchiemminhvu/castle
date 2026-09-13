#ifndef CASTLE_MATH_TRANSFORM_H
#define CASTLE_MATH_TRANSFORM_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/math/angle.h"
#include "castle/math/geometry/point2d.h"
#include "castle/math/geometry/point3d.h"
#include "castle/math/geometry/vector2d.h"
#include "castle/math/geometry/vector3d.h"
#include "castle/math/linalg/matrix.h"
#include "castle/math/linalg/trigonometry.h"
#include "castle/math/linalg/vector.h"

namespace castle
{
namespace math
{

// ============================================================================
// Homogeneous 2D transforms.
//
// Vectors are column vectors and matrices are row-major. The convention is:
//     [ x' ]   [ a c tx ] [ x ]
//     [ y' ] = [ b d ty ] [ y ]
//     [ 1  ]   [ 0 0  1 ] [ 1 ]
// ============================================================================
template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> translation_2d(T x, T y) CASTLE_NOEXCEPT
{
    matrix<T, 3U, 3U> result = identity<T, 3U>();
    result(0U, 2U) = x;
    result(1U, 2U) = y;
    return result;
}

template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> translation_2d(CASTLE_CONST vector<T, 2U>& value) CASTLE_NOEXCEPT
{
    return translation_2d(value[0U], value[1U]);
}

template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> scaling_2d(T x, T y) CASTLE_NOEXCEPT
{
    matrix<T, 3U, 3U> result;
    result(0U, 0U) = x;
    result(1U, 1U) = y;
    result(2U, 2U) = static_cast<T>(1);
    return result;
}

template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> scaling_2d(CASTLE_CONST vector<T, 2U>& value) CASTLE_NOEXCEPT
{
    return scaling_2d(value[0U], value[1U]);
}

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

template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> rotation_2d_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_2d(degrees_to_radians(degrees));
}

template <typename T>
CASTLE_NODISCARD vector<T, 2U> transform_point_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST vector<T, 2U>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> homogeneous(point[0U], point[1U], static_cast<T>(1));
    CASTLE_CONST vector<T, 3U> result = transform * homogeneous;
    CASTLE_ASSERT(result[2U] != static_cast<T>(0),
                  CASTLE_ERROR_GENERIC("castle::math::transform_point_2d: invalid homogeneous coordinate"));

    if (result[2U] == static_cast<T>(1))
    {
        return vector<T, 2U>(result[0U], result[1U]);
    }

    return vector<T, 2U>(
        static_cast<T>(result[0U] / result[2U]),
        static_cast<T>(result[1U] / result[2U]));
}

template <typename T>
CASTLE_NODISCARD vector<T, 2U> transform_vector_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST vector<T, 2U>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> homogeneous(value[0U], value[1U], static_cast<T>(0));
    CASTLE_CONST vector<T, 3U> result = transform * homogeneous;
    return vector<T, 2U>(result[0U], result[1U]);
}

template <typename T>
CASTLE_NODISCARD point2d<T> transform_point_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST point2d<T>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 2U> result = transform_point_2d(
        transform, vector<T, 2U>(point.x(), point.y()));
    return point2d<T>(result[0U], result[1U]);
}

template <typename T>
CASTLE_NODISCARD vector2d<T> transform_vector_2d(
    CASTLE_CONST matrix<T, 3U, 3U>& transform,
    CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 2U> result = transform_vector_2d(
        transform, vector<T, 2U>(value.x(), value.y()));
    return vector2d<T>(result[0U], result[1U]);
}

// ============================================================================
// Homogeneous 3D transforms.
// ============================================================================
template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> translation_3d(T x, T y, T z) CASTLE_NOEXCEPT
{
    matrix<T, 4U, 4U> result = identity<T, 4U>();
    result(0U, 3U) = x;
    result(1U, 3U) = y;
    result(2U, 3U) = z;
    return result;
}

template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> translation_3d(CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    return translation_3d(value[0U], value[1U], value[2U]);
}

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

template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> scaling_3d(CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    return scaling_3d(value[0U], value[1U], value[2U]);
}

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

template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_x_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_x(degrees_to_radians(degrees));
}

template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_y_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_y(degrees_to_radians(degrees));
}

template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_z_degrees(T degrees) CASTLE_NOEXCEPT
{
    return rotation_z(degrees_to_radians(degrees));
}

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

template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_axis_angle_degrees(
    CASTLE_CONST vector<T, 3U>& axis,
    T degrees) CASTLE_NOEXCEPT
{
    return rotation_axis_angle(axis, degrees_to_radians(degrees));
}

template <typename T>
CASTLE_NODISCARD matrix<T, 4U, 4U> rotation_xyz(
    T x_radians,
    T y_radians,
    T z_radians) CASTLE_NOEXCEPT
{
    return rotation_z(z_radians) * rotation_y(y_radians) * rotation_x(x_radians);
}

template <typename T>
CASTLE_NODISCARD matrix<T, 3U, 3U> compose_transform_2d(
    CASTLE_CONST vector<T, 2U>& translation,
    T rotation_radians,
    CASTLE_CONST vector<T, 2U>& scale) CASTLE_NOEXCEPT
{
    return translation_2d(translation) * rotation_2d(rotation_radians) * scaling_2d(scale);
}

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

template <typename T>
CASTLE_NODISCARD vector<T, 3U> transform_point_3d(
    CASTLE_CONST matrix<T, 4U, 4U>& transform,
    CASTLE_CONST vector<T, 3U>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 4U> homogeneous(point[0U], point[1U], point[2U], static_cast<T>(1));
    CASTLE_CONST vector<T, 4U> result = transform * homogeneous;
    CASTLE_ASSERT(result[3U] != static_cast<T>(0),
                  CASTLE_ERROR_GENERIC("castle::math::transform_point_3d: invalid homogeneous coordinate"));

    if (result[3U] == static_cast<T>(1))
    {
        return vector<T, 3U>(result[0U], result[1U], result[2U]);
    }

    return vector<T, 3U>(
        static_cast<T>(result[0U] / result[3U]),
        static_cast<T>(result[1U] / result[3U]),
        static_cast<T>(result[2U] / result[3U]));
}

template <typename T>
CASTLE_NODISCARD vector<T, 3U> transform_vector_3d(
    CASTLE_CONST matrix<T, 4U, 4U>& transform,
    CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 4U> homogeneous(value[0U], value[1U], value[2U], static_cast<T>(0));
    CASTLE_CONST vector<T, 4U> result = transform * homogeneous;
    return vector<T, 3U>(result[0U], result[1U], result[2U]);
}

template <typename T>
CASTLE_NODISCARD point3d<T> transform_point_3d(
    CASTLE_CONST matrix<T, 4U, 4U>& transform,
    CASTLE_CONST point3d<T>& point) CASTLE_NOEXCEPT
{
    CASTLE_CONST vector<T, 3U> result = transform_point_3d(
        transform, vector<T, 3U>(point.x(), point.y(), point.z()));
    return point3d<T>(result[0U], result[1U], result[2U]);
}

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

#endif // CASTLE_MATH_TRANSFORM_H
