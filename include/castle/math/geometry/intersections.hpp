// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Geometric intersection and relation queries.
 *
 * This header provides relation enums and bounded-size result containers for
 * line, plane, and circle intersections in 2D and 3D. Implementations remain
 * header-only, allocation-free, and deterministic, with at most two explicit
 * intersection points returned for finite circle-related queries.
 */
#ifndef CASTLE_MATH_GEOMETRY_INTERSECTIONS_HPP
#define CASTLE_MATH_GEOMETRY_INTERSECTIONS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/math/geometry/circle2d.hpp"
#include "castle/math/geometry/circle3d.hpp"
#include "castle/math/geometry/detail.hpp"
#include "castle/math/geometry/line2d.hpp"
#include "castle/math/geometry/line3d.hpp"
#include "castle/math/geometry/plane3d.hpp"
#include "castle/math/geometry/vector3d.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle/container/array.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Relationship between two infinite 2D lines.
 */
enum class line2d_relation : uint8_t
{
    parallel = 0U,
    coincident,
    intersecting
};

/**
 * @brief Relationship between two infinite 3D lines.
 */
enum class line3d_relation : uint8_t
{
    parallel = 0U,
    coincident,
    intersecting,
    skew
};

/**
 * @brief Relationship between an infinite 3D line and a plane.
 */
enum class line_plane_relation : uint8_t
{
    parallel = 0U,
    contained,
    intersecting
};

/**
 * @brief Relationship between two infinite 3D planes.
 */
enum class plane3d_relation : uint8_t
{
    parallel = 0U,
    coincident,
    intersecting
};

/**
 * @brief Holds up to two 2D intersection points.
 * @tparam T Coordinate type.
 * @note coincident == true signals an infinite family of solutions and leaves count at zero.
 */
template <typename T>
struct intersection_points2d
{
    size_type count;                         ///< Number of valid entries in points.
    container::array<point2d<T>, 2U> points; ///< Returned intersection points.
    bool coincident;                        ///< true when the geometric objects coincide.

    /** @brief Constructs an empty intersection result. */
    CASTLE_CONSTEXPR intersection_points2d() CASTLE_NOEXCEPT
        : count(0U), points(), coincident(false)
    {
    }
};

/**
 * @brief Holds up to two 3D intersection points.
 * @tparam T Coordinate type.
 * @note coincident == true signals an infinite family of solutions and leaves count at zero.
 */
template <typename T>
struct intersection_points3d
{
    size_type count;                         ///< Number of valid entries in points.
    container::array<point3d<T>, 2U> points; ///< Returned intersection points.
    bool coincident;                        ///< true when the geometric objects coincide.

    /** @brief Constructs an empty intersection result. */
    CASTLE_CONSTEXPR intersection_points3d() CASTLE_NOEXCEPT
        : count(0U), points(), coincident(false)
    {
    }
};

/**
 * @brief Intersects two infinite 2D lines.
 * @tparam T Floating-point coordinate type.
 * @param first First line.
 * @param second Second line.
 * @param intersection Output point when the relation is intersecting.
 * @param epsilon Absolute tolerance used for parallel and coincident tests.
 * @return Relation between the two lines.
 */
template <typename T>
CASTLE_NODISCARD line2d_relation intersect_lines(
    CASTLE_CONST line2d<T>& first,
    CASTLE_CONST line2d<T>& second,
    point2d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    CASTLE_CONST vector2d<T> delta = second.origin() - first.origin();
    CASTLE_CONST T denominator = first.direction().cross(second.direction());

    if (detail::near_zero(denominator, epsilon))
    {
        if (detail::near_zero(delta.cross(first.direction()), epsilon))
        {
            return line2d_relation::coincident;
        }
        return line2d_relation::parallel;
    }

    CASTLE_CONST T parameter = static_cast<T>(delta.cross(second.direction()) / denominator);
    intersection = first.point_at(parameter);
    return line2d_relation::intersecting;
}

/**
 * @brief Intersects an infinite 3D line with a plane.
 * @tparam T Floating-point coordinate type.
 * @param line Line to classify.
 * @param plane Plane to classify against.
 * @param intersection Output point when the relation is intersecting.
 * @param epsilon Absolute tolerance applied to the denominator and plane test.
 * @return Relation between the line and plane.
 */
template <typename T>
CASTLE_NODISCARD line_plane_relation intersect_line_plane(
    CASTLE_CONST line3d<T>& line,
    CASTLE_CONST plane3d<T>& plane,
    point3d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    CASTLE_CONST T denominator = line.direction().dot(plane.normal());
    if (detail::near_zero(denominator, epsilon))
    {
        return plane.contains(line.origin(), epsilon)
                   ? line_plane_relation::contained
                   : line_plane_relation::parallel;
    }

    CASTLE_CONST T numerator = plane.normal().dot(plane.point() - line.origin());
    CASTLE_CONST T parameter = static_cast<T>(numerator / denominator);
    intersection = line.point_at(parameter);
    return line_plane_relation::intersecting;
}

/**
 * @brief Intersects two infinite 3D lines.
 * @tparam T Floating-point coordinate type.
 * @param first First line.
 * @param second Second line.
 * @param intersection Output point when the relation is intersecting.
 * @param epsilon Absolute tolerance used by the parallel and coplanarity tests.
 * @return Relation between the two lines.
 * @note Non-parallel lines may still be skew and therefore return no point.
 */
template <typename T>
CASTLE_NODISCARD line3d_relation intersect_lines(
    CASTLE_CONST line3d<T>& first,
    CASTLE_CONST line3d<T>& second,
    point3d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    CASTLE_CONST vector3d<T> direction_cross = first.direction().cross(second.direction());
    CASTLE_CONST vector3d<T> between = second.origin() - first.origin();
    CASTLE_CONST T cross_squared = direction_cross.squared_length();

    if (detail::near_zero(cross_squared, static_cast<T>(epsilon * epsilon)))
    {
        if (between.cross(first.direction()).squared_length() <= static_cast<T>(epsilon * epsilon))
        {
            return line3d_relation::coincident;
        }
        return line3d_relation::parallel;
    }

    CASTLE_CONST T connector_projection = between.dot(direction_cross);
    if (detail::near_zero(connector_projection, epsilon))
    {
        CASTLE_CONST T t = static_cast<T>(between.cross(second.direction()).dot(direction_cross) /
                                   cross_squared);
        intersection = first.point_at(t);
        return line3d_relation::intersecting;
    }

    return line3d_relation::skew;
}

/**
 * @brief Intersects two infinite 3D planes.
 * @tparam T Floating-point coordinate type.
 * @param first First plane.
 * @param second Second plane.
 * @param intersection Output line when the relation is intersecting.
 * @param epsilon Absolute tolerance used to detect parallel or coincident planes.
 * @return Relation between the two planes.
 */
template <typename T>
CASTLE_NODISCARD plane3d_relation intersect_planes(
    CASTLE_CONST plane3d<T>& first,
    CASTLE_CONST plane3d<T>& second,
    line3d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    CASTLE_CONST vector3d<T> direction = first.normal().cross(second.normal());
    if (detail::near_zero(direction.squared_length(), static_cast<T>(epsilon * epsilon)))
    {
        if (detail::near_zero(first.normal().dot(second.point() - first.point()), epsilon))
        {
            return plane3d_relation::coincident;
        }
        return plane3d_relation::parallel;
    }

    CASTLE_CONST T a = first.normal().dot(first.normal());
    CASTLE_CONST T b = first.normal().dot(second.normal());
    CASTLE_CONST T c = second.normal().dot(second.normal());
    CASTLE_CONST T d1 = static_cast<T>(first.normal().x() * first.point().x() +
                                first.normal().y() * first.point().y() +
                                first.normal().z() * first.point().z());
    CASTLE_CONST T d2 = static_cast<T>(second.normal().x() * second.point().x() +
                                second.normal().y() * second.point().y() +
                                second.normal().z() * second.point().z());
    CASTLE_CONST T determinant = static_cast<T>(a * c - b * b);

    CASTLE_CONST T alpha = static_cast<T>((d1 * c - b * d2) / determinant);
    CASTLE_CONST T beta = static_cast<T>((a * d2 - b * d1) / determinant);
    CASTLE_CONST point3d<T> point = point3d<T>(
        static_cast<T>(alpha * first.normal().x() + beta * second.normal().x()),
        static_cast<T>(alpha * first.normal().y() + beta * second.normal().y()),
        static_cast<T>(alpha * first.normal().z() + beta * second.normal().z()));

    intersection = line3d<T>(point, direction);
    return plane3d_relation::intersecting;
}

/**
 * @brief Intersects two 2D circles.
 * @tparam T Floating-point coordinate type.
 * @param first First circle.
 * @param second Second circle.
 * @param epsilon Absolute tolerance used in coincidence, tangent, and separation tests.
 * @return Intersection result containing zero, one, or two points.
 * @note coincident == true indicates matching circles with infinitely many solutions.
 */
template <typename T>
CASTLE_NODISCARD intersection_points2d<T> intersect_circles(
    CASTLE_CONST circle2d<T>& first,
    CASTLE_CONST circle2d<T>& second,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    intersection_points2d<T> result;
    CASTLE_CONST vector2d<T> delta = second.center() - first.center();
    CASTLE_CONST T distance_squared = delta.squared_length();
    CASTLE_CONST T radius_sum = static_cast<T>(first.radius() + second.radius());
    CASTLE_CONST T radius_difference = static_cast<T>(abs(first.radius() - second.radius())); // LCOV_EXCL_BR_LINE

    if (detail::near_zero(distance_squared, epsilon * epsilon))
    {
        if (detail::near_zero(static_cast<T>(first.radius() - second.radius()), epsilon))
        {
            result.coincident = true;
        }
        return result;
    }

    CASTLE_CONST T distance = sqrt_real(distance_squared);
    if (distance > static_cast<T>(radius_sum + epsilon) ||
        distance < static_cast<T>(radius_difference - epsilon))
    {
        return result;
    }

    CASTLE_CONST T a = static_cast<T>((first.radius() * first.radius() -
                                second.radius() * second.radius() +
                                distance_squared) /
                               (static_cast<T>(2) * distance));
    CASTLE_CONST T h_squared_raw = static_cast<T>(first.radius() * first.radius() - a * a);
    CASTLE_CONST T h_squared = (h_squared_raw < T{}) ? T{} : h_squared_raw;
    CASTLE_CONST T h = sqrt_real(h_squared);

    CASTLE_CONST T ux = static_cast<T>(delta.x() / distance);
    CASTLE_CONST T uy = static_cast<T>(delta.y() / distance);
    CASTLE_CONST point2d<T> base = first.center() + vector2d<T>(
        static_cast<T>(ux * a),
        static_cast<T>(uy * a));

    if (h <= epsilon)
    {
        result.points[0U] = base;
        result.count = 1U;
        return result;
    }

    result.points[0U] = base + vector2d<T>(static_cast<T>(-uy * h), static_cast<T>(ux * h));
    result.points[1U] = base + vector2d<T>(static_cast<T>(uy * h), static_cast<T>(-ux * h));
    result.count = 2U;
    return result;
}

/**
 * @brief Intersects an infinite 2D line with a circle.
 * @tparam T Floating-point coordinate type.
 * @param line Line to intersect.
 * @param circle Circle to intersect.
 * @param epsilon Absolute tolerance used for tangency and degenerate-line handling.
 * @return Intersection result containing zero, one, or two points.
 */
template <typename T>
CASTLE_NODISCARD intersection_points2d<T> intersect_line_circle(
    CASTLE_CONST line2d<T>& line,
    CASTLE_CONST circle2d<T>& circle,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    intersection_points2d<T> result;
    if (line.degenerate())
    {
        if (circle.on_circle(line.origin(), epsilon))
        {
            result.points[0U] = line.origin();
            result.count = 1U;
        }
        return result;
    }

    CASTLE_CONST vector2d<T> offset = circle.center() - line.origin();
    CASTLE_CONST T direction_squared = line.direction().squared_length();
    CASTLE_CONST T t = static_cast<T>(offset.dot(line.direction()) / direction_squared);
    CASTLE_CONST point2d<T> closest = line.point_at(t);
    CASTLE_CONST vector2d<T> radial = circle.center() - closest;
    CASTLE_CONST T distance_squared = radial.squared_length();
    CASTLE_CONST T radius = circle.radius();
    CASTLE_CONST T radius_squared = static_cast<T>(radius * radius);

    if (distance_squared > static_cast<T>((radius + epsilon) * (radius + epsilon)))
    {
        return result;
    }

    CASTLE_CONST T h_squared_raw = static_cast<T>(radius_squared - distance_squared);
    CASTLE_CONST T h_squared = (h_squared_raw < T{}) ? T{} : h_squared_raw;
    CASTLE_CONST T h = sqrt_real(h_squared);
    CASTLE_CONST T direction_length = sqrt_real(direction_squared);
    CASTLE_CONST vector2d<T> unit_direction = line.direction() / direction_length;

    if (h <= epsilon)
    {
        result.points[0U] = closest;
        result.count = 1U;
        return result;
    }

    result.points[0U] = closest + unit_direction * h;
    result.points[1U] = closest - unit_direction * h;
    result.count = 2U;
    return result;
}

/**
 * @brief Intersects an infinite 3D line with a 3D circle.
 * @tparam T Floating-point coordinate type.
 * @param line Line to intersect.
 * @param circle Circle to intersect.
 * @param epsilon Absolute tolerance used for plane and tangency tests.
 * @return Intersection result containing zero, one, or two points.
 */
template <typename T>
CASTLE_NODISCARD intersection_points3d<T> intersect_line_circle(
    CASTLE_CONST line3d<T>& line,
    CASTLE_CONST circle3d<T>& circle,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    intersection_points3d<T> result;

    if (line.degenerate())
    {
        if (circle.on_circle(line.origin(), epsilon))
        {
            result.points[0U] = line.origin();
            result.count = 1U;
        }
        return result;
    }

    point3d<T> plane_intersection;
    CASTLE_CONST line_plane_relation plane_relation = intersect_line_plane(
        line, circle.plane(), plane_intersection, epsilon);
    if (plane_relation == line_plane_relation::intersecting)
    {
        if (circle.on_circle(plane_intersection, epsilon))
        {
            result.points[0U] = plane_intersection;
            result.count = 1U;
        }
        return result;
    }

    if (plane_relation == line_plane_relation::parallel)
    {
        return result;
    }

    CASTLE_CONST vector3d<T> from_center = circle.center() - line.origin();
    CASTLE_CONST T direction_squared = line.direction().squared_length();
    CASTLE_CONST T parameter = static_cast<T>(from_center.dot(line.direction()) / direction_squared);
    CASTLE_CONST point3d<T> closest = line.point_at(parameter);
    CASTLE_CONST vector3d<T> radial = circle.center() - closest;
    CASTLE_CONST T distance_squared = radial.squared_length();
    CASTLE_CONST T radius = circle.radius();
    CASTLE_CONST T expanded_radius = static_cast<T>(radius + epsilon);

    if (distance_squared > static_cast<T>(expanded_radius * expanded_radius))
    {
        return result;
    }

    CASTLE_CONST T h_squared_raw = static_cast<T>(radius * radius - distance_squared);
    CASTLE_CONST T h_squared = (h_squared_raw < T{}) ? T{} : h_squared_raw;
    CASTLE_CONST T h = sqrt_real(h_squared);
    CASTLE_CONST T direction_length = sqrt_real(direction_squared);
    CASTLE_CONST vector3d<T> unit_direction = line.direction() / direction_length;

    if (h <= epsilon)
    {
        result.points[0U] = closest;
        result.count = 1U;
        return result;
    }

    result.points[0U] = closest + unit_direction * h;
    result.points[1U] = closest - unit_direction * h;
    result.count = 2U;
    return result;
}

/**
 * @brief Intersects two 3D circles.
 * @tparam T Floating-point coordinate type.
 * @param first First circle.
 * @param second Second circle.
 * @param epsilon Absolute tolerance used for plane and circle tests.
 * @return Intersection result containing zero, one, or two points.
 * @note coincident == true indicates coplanar circles with infinitely many shared points.
 */
template <typename T>
CASTLE_NODISCARD intersection_points3d<T> intersect_circles(
    CASTLE_CONST circle3d<T>& first,
    CASTLE_CONST circle3d<T>& second,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    intersection_points3d<T> result;
    line3d<T> plane_intersection;
    CASTLE_CONST plane3d_relation relation = intersect_planes(
        first.plane(), second.plane(), plane_intersection, epsilon);

    if (relation == plane3d_relation::parallel)
    {
        return result;
    }

    if (relation == plane3d_relation::intersecting)
    {
        CASTLE_CONST intersection_points3d<T> candidates = intersect_line_circle(
            plane_intersection, first, epsilon);
        for (size_type i = 0U; i < candidates.count; ++i)
        {
            if (second.on_circle(candidates.points[i], epsilon))
            {
                result.points[result.count] = candidates.points[i];
                ++result.count;
            }
        }
        return result;
    }

    CASTLE_CONST vector3d<T> normal = first.normal().normalized();
    vector3d<T> basis_u;
    if (abs(normal.x()) > abs(normal.z()))
    {
        basis_u = vector3d<T>(-normal.y(), normal.x(), T{}).normalized();
    }
    else
    {
        basis_u = vector3d<T>(T{}, -normal.z(), normal.y()).normalized();
    }
    CASTLE_CONST vector3d<T> basis_v = normal.cross(basis_u);
    CASTLE_CONST vector3d<T> delta = second.center() - first.center();

    CASTLE_CONST circle2d<T> first_2d(point2d<T>(T{}, T{}), first.radius());
    CASTLE_CONST circle2d<T> second_2d(
        point2d<T>(delta.dot(basis_u), delta.dot(basis_v)),
        second.radius());
    CASTLE_CONST intersection_points2d<T> planar = intersect_circles(first_2d, second_2d, epsilon);

    result.coincident = planar.coincident;
    for (size_type i = 0U; i < planar.count; ++i)
    {
        CASTLE_CONST point2d<T> point = planar.points[i];
        result.points[result.count] = first.center() +
            basis_u * point.x() + basis_v * point.y();
        ++result.count;
    }
    return result;
}

}
}

#endif
