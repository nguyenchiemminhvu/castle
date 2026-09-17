#ifndef CASTLE_MATH_GEOMETRY_INTERSECTIONS_H
#define CASTLE_MATH_GEOMETRY_INTERSECTIONS_H

#include "castle/core/compiler.h"
#include "castle/core/type_ranges.h"
#include "castle/core/types.h"
#include "castle/math/geometry/circle2d.h"
#include "castle/math/geometry/circle3d.h"
#include "castle/math/geometry/detail.h"
#include "castle/math/geometry/line2d.h"
#include "castle/math/geometry/line3d.h"
#include "castle/math/geometry/plane3d.h"
#include "castle/math/geometry/vector3d.h"
#include "castle/math/sqrt_real.h"
#include "castle/container/array.h"

namespace castle
{
namespace math
{

enum class line2d_relation : uint8_t
{
    parallel = 0U,
    coincident,
    intersecting
};

enum class line3d_relation : uint8_t
{
    parallel = 0U,
    coincident,
    intersecting,
    skew
};

enum class line_plane_relation : uint8_t
{
    parallel = 0U,
    contained,
    intersecting
};

enum class plane3d_relation : uint8_t
{
    parallel = 0U,
    coincident,
    intersecting
};

template <typename T>
struct intersection_points2d
{
    size_type count;
    container::array<point2d<T>, 2U> points;
    bool coincident;

    CASTLE_CONSTEXPR intersection_points2d() CASTLE_NOEXCEPT
        : count(0U), points(), coincident(false)
    {
    }
};

template <typename T>
struct intersection_points3d
{
    size_type count;
    container::array<point3d<T>, 2U> points;
    bool coincident;

    CASTLE_CONSTEXPR intersection_points3d() CASTLE_NOEXCEPT
        : count(0U), points(), coincident(false)
    {
    }
};

// ============================================================================
// Intersection of two infinite 2D lines.
//
// The cross-product denominator is tested before division, so parallel and
// coincident lines are handled without numerical division-by-zero.
//
// O(1) time, O(1) storage.
// ============================================================================
template <typename T>
CASTLE_NODISCARD line2d_relation intersect_lines(
    const line2d<T>& first,
    const line2d<T>& second,
    point2d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    const vector2d<T> delta = second.origin() - first.origin();
    const T denominator = first.direction().cross(second.direction());

    if (detail::near_zero(denominator, epsilon))
    {
        if (detail::near_zero(delta.cross(first.direction()), epsilon))
        {
            return line2d_relation::coincident;
        }
        return line2d_relation::parallel;
    }

    const T parameter = static_cast<T>(delta.cross(second.direction()) / denominator);
    intersection = first.point_at(parameter);
    return line2d_relation::intersecting;
}

// ============================================================================
// Intersection of an infinite 3D line with a plane.
//
// Returns false when the line is parallel to the plane.
// O(1) time, O(1) storage.
// ============================================================================
template <typename T>
CASTLE_NODISCARD line_plane_relation intersect_line_plane(
    const line3d<T>& line,
    const plane3d<T>& plane,
    point3d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    const T denominator = line.direction().dot(plane.normal());
    if (detail::near_zero(denominator, epsilon))
    {
        return plane.contains(line.origin(), epsilon)
                   ? line_plane_relation::contained
                   : line_plane_relation::parallel;
    }

    const T numerator = plane.normal().dot(plane.point() - line.origin());
    const T parameter = static_cast<T>(numerator / denominator);
    intersection = line.point_at(parameter);
    return line_plane_relation::intersecting;
}

// ============================================================================
// Intersection of two infinite 3D lines.
//
// A pair of non-parallel lines can still be skew. The shortest connector is
// therefore used to distinguish true intersection from a skew relationship.
// O(1) time, O(1) storage.
// ============================================================================
template <typename T>
CASTLE_NODISCARD line3d_relation intersect_lines(
    const line3d<T>& first,
    const line3d<T>& second,
    point3d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    const vector3d<T> direction_cross = first.direction().cross(second.direction());
    const vector3d<T> between = second.origin() - first.origin();
    const T cross_squared = direction_cross.squared_length();

    if (detail::near_zero(cross_squared, static_cast<T>(epsilon * epsilon)))
    {
        if (between.cross(first.direction()).squared_length() <= static_cast<T>(epsilon * epsilon))
        {
            return line3d_relation::coincident;
        }
        return line3d_relation::parallel;
    }

    const T connector_projection = between.dot(direction_cross);
    if (detail::near_zero(connector_projection, epsilon))
    {
        const T t = static_cast<T>(between.cross(second.direction()).dot(direction_cross) /
                                   cross_squared);
        intersection = first.point_at(t);
        return line3d_relation::intersecting;
    }

    return line3d_relation::skew;
}

// ============================================================================
// Intersection of two infinite 3D planes.
//
// When the planes are not parallel, their intersection is an infinite line.
// O(1) time, O(1) storage.
// ============================================================================
template <typename T>
CASTLE_NODISCARD plane3d_relation intersect_planes(
    const plane3d<T>& first,
    const plane3d<T>& second,
    line3d<T>& intersection,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "intersection requires a floating-point type");
    const vector3d<T> direction = first.normal().cross(second.normal());
    if (detail::near_zero(direction.squared_length(), static_cast<T>(epsilon * epsilon)))
    {
        if (detail::near_zero(first.normal().dot(second.point() - first.point()), epsilon))
        {
            return plane3d_relation::coincident;
        }
        return plane3d_relation::parallel;
    }

    // Solve the two plane equations using p = alpha*n1 + beta*n2 on the
    // intersection line. This avoids a matrix library and uses only dot/cross.
    const T a = first.normal().dot(first.normal());
    const T b = first.normal().dot(second.normal());
    const T c = second.normal().dot(second.normal());
    const T d1 = static_cast<T>(first.normal().x() * first.point().x() +
                                first.normal().y() * first.point().y() +
                                first.normal().z() * first.point().z());
    const T d2 = static_cast<T>(second.normal().x() * second.point().x() +
                                second.normal().y() * second.point().y() +
                                second.normal().z() * second.point().z());
    const T determinant = static_cast<T>(a * c - b * b);

    const T alpha = static_cast<T>((d1 * c - b * d2) / determinant);
    const T beta = static_cast<T>((a * d2 - b * d1) / determinant);
    const point3d<T> point = point3d<T>(
        static_cast<T>(alpha * first.normal().x() + beta * second.normal().x()),
        static_cast<T>(alpha * first.normal().y() + beta * second.normal().y()),
        static_cast<T>(alpha * first.normal().z() + beta * second.normal().z()));

    intersection = line3d<T>(point, direction);
    return plane3d_relation::intersecting;
}

// ============================================================================
// Intersection of two circles in 2D.
//
// The standard center-distance construction is optimal here: O(1) time, no
// iteration, and at most two result points.
// ============================================================================
template <typename T>
CASTLE_NODISCARD intersection_points2d<T> intersect_circles(
    const circle2d<T>& first,
    const circle2d<T>& second,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    intersection_points2d<T> result;
    const vector2d<T> delta = second.center() - first.center();
    const T distance_squared = delta.squared_length();
    const T radius_sum = static_cast<T>(first.radius() + second.radius());
    const T radius_difference = static_cast<T>(abs(first.radius() - second.radius()));

    if (detail::near_zero(distance_squared, epsilon * epsilon))
    {
        if (detail::near_zero(static_cast<T>(first.radius() - second.radius()), epsilon))
        {
            result.coincident = true;
        }
        return result;
    }

    const T distance = sqrt_real(distance_squared);
    if (distance > static_cast<T>(radius_sum + epsilon) ||
        distance < static_cast<T>(radius_difference - epsilon))
    {
        return result;
    }

    const T a = static_cast<T>((first.radius() * first.radius() -
                                second.radius() * second.radius() +
                                distance_squared) /
                               (static_cast<T>(2) * distance));
    const T h_squared_raw = static_cast<T>(first.radius() * first.radius() - a * a);
    const T h_squared = (h_squared_raw < T{}) ? T{} : h_squared_raw;
    const T h = sqrt_real(h_squared);

    const T ux = static_cast<T>(delta.x() / distance);
    const T uy = static_cast<T>(delta.y() / distance);
    const point2d<T> base = first.center() + vector2d<T>(
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

// ============================================================================
// Intersection of a 2D line and a circle.
// ============================================================================
template <typename T>
CASTLE_NODISCARD intersection_points2d<T> intersect_line_circle(
    const line2d<T>& line,
    const circle2d<T>& circle,
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

    const vector2d<T> offset = circle.center() - line.origin();
    const T direction_squared = line.direction().squared_length();
    const T t = static_cast<T>(offset.dot(line.direction()) / direction_squared);
    const point2d<T> closest = line.point_at(t);
    const vector2d<T> radial = circle.center() - closest;
    const T distance_squared = radial.squared_length();
    const T radius = circle.radius();
    const T radius_squared = static_cast<T>(radius * radius);

    if (distance_squared > static_cast<T>((radius + epsilon) * (radius + epsilon)))
    {
        return result;
    }

    const T h_squared_raw = static_cast<T>(radius_squared - distance_squared);
    const T h_squared = (h_squared_raw < T{}) ? T{} : h_squared_raw;
    const T h = sqrt_real(h_squared);
    const T direction_length = sqrt_real(direction_squared);
    const vector2d<T> unit_direction = line.direction() / direction_length;

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

// ============================================================================
// Intersection of an infinite 3D line and a circle in a plane.
// ============================================================================
template <typename T>
CASTLE_NODISCARD intersection_points3d<T> intersect_line_circle(
    const line3d<T>& line,
    const circle3d<T>& circle,
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
    const line_plane_relation plane_relation = intersect_line_plane(
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

    // The line lies in the circle plane. Project the center onto the line and
    // construct one or two points from the perpendicular distance.
    const vector3d<T> from_center = circle.center() - line.origin();
    const T direction_squared = line.direction().squared_length();
    const T parameter = static_cast<T>(from_center.dot(line.direction()) / direction_squared);
    const point3d<T> closest = line.point_at(parameter);
    const vector3d<T> radial = circle.center() - closest;
    const T distance_squared = radial.squared_length();
    const T radius = circle.radius();
    const T expanded_radius = static_cast<T>(radius + epsilon);

    if (distance_squared > static_cast<T>(expanded_radius * expanded_radius))
    {
        return result;
    }

    const T h_squared_raw = static_cast<T>(radius * radius - distance_squared);
    const T h_squared = (h_squared_raw < T{}) ? T{} : h_squared_raw;
    const T h = sqrt_real(h_squared);
    const T direction_length = sqrt_real(direction_squared);
    const vector3d<T> unit_direction = line.direction() / direction_length;

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

// ============================================================================
// Intersection of two coplanar or non-coplanar 3D circles.
// ============================================================================
template <typename T>
CASTLE_NODISCARD intersection_points3d<T> intersect_circles(
    const circle3d<T>& first,
    const circle3d<T>& second,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    intersection_points3d<T> result;
    line3d<T> plane_intersection;
    const plane3d_relation relation = intersect_planes(
        first.plane(), second.plane(), plane_intersection, epsilon);

    if (relation == plane3d_relation::parallel)
    {
        return result;
    }

    if (relation == plane3d_relation::intersecting)
    {
        const intersection_points3d<T> candidates = intersect_line_circle(
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

    // The circles share one plane. Build an orthonormal 2D basis inside it,
    // intersect the corresponding 2D circles, then map the results back.
    const vector3d<T> normal = first.normal().normalized();
    vector3d<T> basis_u;
    if (abs(normal.x()) > abs(normal.z()))
    {
        basis_u = vector3d<T>(-normal.y(), normal.x(), T{}).normalized();
    }
    else
    {
        basis_u = vector3d<T>(T{}, -normal.z(), normal.y()).normalized();
    }
    const vector3d<T> basis_v = normal.cross(basis_u);
    const vector3d<T> delta = second.center() - first.center();

    const circle2d<T> first_2d(point2d<T>(T{}, T{}), first.radius());
    const circle2d<T> second_2d(
        point2d<T>(delta.dot(basis_u), delta.dot(basis_v)),
        second.radius());
    const intersection_points2d<T> planar = intersect_circles(first_2d, second_2d, epsilon);

    result.coincident = planar.coincident;
    for (size_type i = 0U; i < planar.count; ++i)
    {
        const point2d<T> point = planar.points[i];
        result.points[result.count] = first.center() +
            basis_u * point.x() + basis_v * point.y();
        ++result.count;
    }
    return result;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_INTERSECTIONS_H
