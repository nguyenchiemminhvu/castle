#ifndef CASTLE_MATH_GEOMETRY_ALGORITHMS_H
#define CASTLE_MATH_GEOMETRY_ALGORITHMS_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/container/array.h"
#include "castle/math/geometry/detail.h"
#include "castle/math/geometry/line2d.h"
#include "castle/math/geometry/line3d.h"
#include "castle/math/hypot.h"
#include "castle/math/geometry/polygon2d.h"
#include "castle/math/square.h"
#include "castle/math/sqrt_real.h"

namespace castle
{
namespace math
{

// ============================================================================
// Squared distance between points.
//
// Squared distance avoids an unnecessary square root and is therefore the
// preferred primitive for nearest-neighbor and collision comparisons.
// O(1) time, O(1) storage.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T squared_distance(
    CASTLE_CONST point2d<T>& first,
    CASTLE_CONST point2d<T>& second) CASTLE_NOEXCEPT
{
    CASTLE_CONST T dx = static_cast<T>(first.x() - second.x());
    CASTLE_CONST T dy = static_cast<T>(first.y() - second.y());
    return static_cast<T>(dx * dx + dy * dy);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T squared_distance(
    CASTLE_CONST point3d<T>& first,
    CASTLE_CONST point3d<T>& second) CASTLE_NOEXCEPT
{
    CASTLE_CONST T dx = static_cast<T>(first.x() - second.x());
    CASTLE_CONST T dy = static_cast<T>(first.y() - second.y());
    CASTLE_CONST T dz = static_cast<T>(first.z() - second.z());
    return static_cast<T>(dx * dx + dy * dy + dz * dz);
}

// ============================================================================
// Euclidean distance between points.
//
// Squared distance is preferred when only ordering/comparison is required.
// This form is for callers that need the physical distance value.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
distance(CASTLE_CONST point2d<T>& first, CASTLE_CONST point2d<T>& second) CASTLE_NOEXCEPT
{
    return hypot(static_cast<T>(first.x() - second.x()),
                 static_cast<T>(first.y() - second.y()));
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
distance(CASTLE_CONST point3d<T>& first, CASTLE_CONST point3d<T>& second) CASTLE_NOEXCEPT
{
    return hypot(static_cast<T>(first.x() - second.x()),
                 static_cast<T>(first.y() - second.y()),
                 static_cast<T>(first.z() - second.z()));
}

// ============================================================================
// Nearest point on an infinite 2D line.
//
// Projection is the optimal O(1) solution; no iterative closest-point search
// is required.
// ============================================================================
template <typename T>
CASTLE_NODISCARD point2d<T> nearest_point(
    CASTLE_CONST point2d<T>& point,
    CASTLE_CONST line2d<T>& line) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "nearest_point on a line requires a floating-point type");
    if (line.degenerate())
    {
        return line.origin();
    }

    CASTLE_CONST vector2d<T> delta = point - line.origin();
    CASTLE_CONST T direction_squared = line.direction().squared_length();
    CASTLE_CONST T parameter = static_cast<T>(delta.dot(line.direction()) / direction_squared);
    return line.point_at(parameter);
}

// ============================================================================
// Nearest point on an infinite 3D line.
// ============================================================================
template <typename T>
CASTLE_NODISCARD point3d<T> nearest_point(
    CASTLE_CONST point3d<T>& point,
    CASTLE_CONST line3d<T>& line) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "nearest_point on a line requires a floating-point type");
    if (line.degenerate())
    {
        return line.origin();
    }

    CASTLE_CONST vector3d<T> delta = point - line.origin();
    CASTLE_CONST T direction_squared = line.direction().squared_length();
    CASTLE_CONST T parameter = static_cast<T>(delta.dot(line.direction()) / direction_squared);
    return line.point_at(parameter);
}

// ============================================================================
// Nearest point on a finite 2D segment.
//
// The line projection is clamped to [0, 1]. O(1) time, O(1) storage.
// ============================================================================
template <typename T>
CASTLE_NODISCARD point2d<T> nearest_point_on_segment(
    CASTLE_CONST point2d<T>& point,
    CASTLE_CONST point2d<T>& first,
    CASTLE_CONST point2d<T>& second) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "nearest_point_on_segment requires a floating-point type");
    CASTLE_CONST vector2d<T> direction = second - first;
    CASTLE_CONST T direction_squared = direction.squared_length();
    if (direction_squared == T{})
    {
        return first;
    }

    CASTLE_CONST vector2d<T> delta = point - first;
    T parameter = static_cast<T>(delta.dot(direction) / direction_squared);
    if (parameter < T{})
    {
        parameter = T{};
    }
    else if (parameter > static_cast<T>(1))
    {
        parameter = static_cast<T>(1);
    }
    return first + direction * parameter;
}

// ============================================================================
// Nearest point on a finite 3D segment.
// ============================================================================
template <typename T>
CASTLE_NODISCARD point3d<T> nearest_point_on_segment(
    CASTLE_CONST point3d<T>& point,
    CASTLE_CONST point3d<T>& first,
    CASTLE_CONST point3d<T>& second) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "nearest_point_on_segment requires a floating-point type");
    CASTLE_CONST vector3d<T> direction = second - first;
    CASTLE_CONST T direction_squared = direction.squared_length();
    if (direction_squared == T{})
    {
        return first;
    }

    CASTLE_CONST vector3d<T> delta = point - first;
    T parameter = static_cast<T>(delta.dot(direction) / direction_squared);
    if (parameter < T{})
    {
        parameter = T{};
    }
    else if (parameter > static_cast<T>(1))
    {
        parameter = static_cast<T>(1);
    }
    return first + direction * parameter;
}

template <typename T>
struct nearest_point_result2d
{
    bool found;
    size_type index;
    point2d<T> point;
    T distance_squared;

    CASTLE_CONSTEXPR nearest_point_result2d() CASTLE_NOEXCEPT
        : found(false), index(0U), point(), distance_squared(T{})
    {
    }
};

template <typename T>
struct nearest_point_result3d
{
    bool found;
    size_type index;
    point3d<T> point;
    T distance_squared;

    CASTLE_CONSTEXPR nearest_point_result3d() CASTLE_NOEXCEPT
        : found(false), index(0U), point(), distance_squared(T{})
    {
    }
};

// ============================================================================
// Find the nearest point in a fixed array.
//
// A linear scan is optimal for an unsorted embedded array: O(N) time, O(1)
// extra storage, deterministic and branch-light. Squared distance avoids sqrt.
// ============================================================================
template <typename T, size_type N>
CASTLE_NODISCARD nearest_point_result2d<T> nearest_point(
    CASTLE_CONST container::array<point2d<T>, N>& points,
    CASTLE_CONST point2d<T>& target) CASTLE_NOEXCEPT
{
    nearest_point_result2d<T> result;
    if (N == 0U)
    {
        return result;
    }

    result.found = true;
    result.index = 0U;
    result.point = points[0U];
    result.distance_squared = squared_distance(points[0U], target);

    for (size_type i = 1U; i < N; ++i)
    {
        CASTLE_CONST T candidate_distance = squared_distance(points[i], target);
        if (candidate_distance < result.distance_squared)
        {
            result.index = i;
            result.point = points[i];
            result.distance_squared = candidate_distance;
        }
    }
    return result;
}

template <typename T, size_type N>
CASTLE_NODISCARD nearest_point_result3d<T> nearest_point(
    CASTLE_CONST container::array<point3d<T>, N>& points,
    CASTLE_CONST point3d<T>& target) CASTLE_NOEXCEPT
{
    nearest_point_result3d<T> result;
    if (N == 0U)
    {
        return result;
    }

    result.found = true;
    result.index = 0U;
    result.point = points[0U];
    result.distance_squared = squared_distance(points[0U], target);

    for (size_type i = 1U; i < N; ++i)
    {
        CASTLE_CONST T candidate_distance = squared_distance(points[i], target);
        if (candidate_distance < result.distance_squared)
        {
            result.index = i;
            result.point = points[i];
            result.distance_squared = candidate_distance;
        }
    }
    return result;
}

// ============================================================================
// Point-on-segment test.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool point_on_segment(
    CASTLE_CONST point2d<T>& point,
    CASTLE_CONST point2d<T>& first,
    CASTLE_CONST point2d<T>& second,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    CASTLE_CONST detail::geometry_calc_type<T> dx = static_cast<detail::geometry_calc_type<T>>(second.x() - first.x());
    CASTLE_CONST detail::geometry_calc_type<T> dy = static_cast<detail::geometry_calc_type<T>>(second.y() - first.y());
    CASTLE_CONST detail::geometry_calc_type<T> px = static_cast<detail::geometry_calc_type<T>>(point.x() - first.x());
    CASTLE_CONST detail::geometry_calc_type<T> py = static_cast<detail::geometry_calc_type<T>>(point.y() - first.y());
    CASTLE_CONST detail::geometry_calc_type<T> cross_value = px * dy - py * dx;
    if (cross_value < -static_cast<detail::geometry_calc_type<T>>(epsilon) ||
        cross_value > static_cast<detail::geometry_calc_type<T>>(epsilon))
    {
        return false;
    }

    CASTLE_CONST detail::geometry_calc_type<T> length_squared = dx * dx + dy * dy;
    if (length_squared == static_cast<detail::geometry_calc_type<T>>(0))
    {
        return (px * px + py * py) <=
               static_cast<detail::geometry_calc_type<T>>(epsilon * epsilon);
    }

    CASTLE_CONST detail::geometry_calc_type<T> dot_value = px * dx + py * dy;
    return dot_value >= -static_cast<detail::geometry_calc_type<T>>(epsilon) &&
           dot_value <= length_squared + static_cast<detail::geometry_calc_type<T>>(epsilon);
}

// ============================================================================
// Point-in-polygon using the ray-casting form of the Jordan curve theorem.
//
// Boundary points are classified as inside. The implementation uses a
// division-free crossing test, which is attractive on microcontrollers because
// it avoids a floating-point divide in the O(N) hot loop.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool point_in_polygon(
    CASTLE_CONST point2d<T>& point,
    CASTLE_CONST point2d<T>* vertices,
    size_type count,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    static_assert(meta::is_signed<T>::value || meta::is_floating_point<T>::value,
                  "point_in_polygon requires signed or floating-point coordinates");

    if (count < 3U || vertices == nullptr)
    {
        return false;
    }

    bool inside = false;
    size_type previous = count - 1U;

    for (size_type current = 0U; current < count; ++current)
    {
        CASTLE_CONST point2d<T>& a = vertices[previous];
        CASTLE_CONST point2d<T>& b = vertices[current];

        if (point_on_segment(point, a, b, epsilon))
        {
            return true;
        }

        CASTLE_CONST bool crosses_scanline = ((a.y() > point.y()) != (b.y() > point.y()));
        if (crosses_scanline)
        {
            using calc_type = detail::geometry_calc_type<T>;
            CASTLE_CONST calc_type lhs =
                static_cast<calc_type>(point.x() - a.x()) *
                static_cast<calc_type>(b.y() - a.y());
            CASTLE_CONST calc_type rhs =
                static_cast<calc_type>(point.y() - a.y()) *
                static_cast<calc_type>(b.x() - a.x());
            CASTLE_CONST bool hit = (b.y() > a.y()) ? (lhs < rhs) : (lhs > rhs);
            if (hit)
            {
                inside = !inside;
            }
        }

        previous = current;
    }

    return inside;
}

template <typename T, size_type Capacity>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool point_in_polygon(
    CASTLE_CONST point2d<T>& point,
    CASTLE_CONST polygon2d<T, Capacity>& polygon,
    T epsilon = T{}) CASTLE_NOEXCEPT
{
    return point_in_polygon(point, polygon.data(), polygon.size(), epsilon);
}

// ============================================================================
// Convex hull of a fixed array of 2D points.
//
// A monotone-chain hull is used: O(N log N) time and O(N) fixed workspace.
// Sorting is implemented locally rather than pulling in the STL. Collinear
// points on a hull edge are removed, leaving the extreme endpoints only.
// ============================================================================
namespace detail
{

template <typename T>
CASTLE_CONSTEXPR bool point_less(CASTLE_CONST point2d<T>& first, CASTLE_CONST point2d<T>& second) CASTLE_NOEXCEPT
{
    return (first.x() < second.x()) ||
           ((first.x() == second.x()) && (first.y() < second.y()));
}

template <typename T>
CASTLE_CONSTEXPR geometry_calc_type<T> orientation(CASTLE_CONST point2d<T>& a,
                                                   CASTLE_CONST point2d<T>& b,
                                                   CASTLE_CONST point2d<T>& c) CASTLE_NOEXCEPT
{
    using calc_type = geometry_calc_type<T>;
    CASTLE_CONST calc_type ab_x = static_cast<calc_type>(b.x()) - static_cast<calc_type>(a.x());
    CASTLE_CONST calc_type ab_y = static_cast<calc_type>(b.y()) - static_cast<calc_type>(a.y());
    CASTLE_CONST calc_type ac_x = static_cast<calc_type>(c.x()) - static_cast<calc_type>(a.x());
    CASTLE_CONST calc_type ac_y = static_cast<calc_type>(c.y()) - static_cast<calc_type>(a.y());
    return static_cast<calc_type>(ab_x * ac_y - ab_y * ac_x);
}

template <typename T, size_type N>
CASTLE_CONSTEXPR void heap_sift_down(container::array<point2d<T>, N>& values,
                                     size_type start,
                                     size_type count) CASTLE_NOEXCEPT
{
    size_type root = start;
    while (root < count / 2U)
    {
        size_type child = static_cast<size_type>(root * 2U + 1U);
        if (child + 1U < count && point_less(values[child], values[child + 1U]))
        {
            ++child;
        }
        if (!point_less(values[root], values[child]))
        {
            return;
        }

        CASTLE_CONST point2d<T> temporary = values[root];
        values[root] = values[child];
        values[child] = temporary;
        root = child;
    }
}

template <typename T, size_type N>
CASTLE_CONSTEXPR void heap_sort(container::array<point2d<T>, N>& values,
                                size_type count) CASTLE_NOEXCEPT
{
    if (count < 2U)
    {
        return;
    }

    for (size_type start = count / 2U; start > 0U; --start)
    {
        heap_sift_down(values, static_cast<size_type>(start - 1U), count);
    }

    for (size_type end = count - 1U; end > 0U; --end)
    {
        CASTLE_CONST point2d<T> temporary = values[0U];
        values[0U] = values[end];
        values[end] = temporary;
        heap_sift_down(values, 0U, end);
    }
}

template <typename T, size_type N>
CASTLE_CONSTEXPR void copy_unique(CASTLE_CONST container::array<point2d<T>, N>& sorted,
                                  size_type count,
                                  container::array<point2d<T>, N>& unique,
                                  size_type& unique_count) CASTLE_NOEXCEPT
{
    unique_count = 0U;
    for (size_type i = 0U; i < count; ++i)
    {
        if (unique_count == 0U || sorted[i] != unique[unique_count - 1U])
        {
            unique[unique_count] = sorted[i];
            ++unique_count;
        }
    }
}

} // namespace detail

template <typename T, size_type N>
CASTLE_NODISCARD CASTLE_CONSTEXPR status convex_hull(
    CASTLE_CONST container::array<point2d<T>, N>& input,
    polygon2d<T, N>& output) CASTLE_NOEXCEPT
{
    static_assert(meta::is_signed<T>::value || meta::is_floating_point<T>::value,
                  "convex_hull requires signed or floating-point coordinates");

    output.clear();
    if (N == 0U)
    {
        return status::ok;
    }

    container::array<point2d<T>, N> sorted = input;
    container::array<point2d<T>, N> unique;
    detail::heap_sort(sorted, N);

    size_type unique_count = 0U;
    detail::copy_unique(sorted, N, unique, unique_count);
    if (unique_count == 1U)
    {
        return output.push_back(unique[0U]);
    }
    if (unique_count == 2U)
    {
        output.push_back(unique[0U]);
        return output.push_back(unique[1U]);
    }

    container::array<point2d<T>, N> hull;
    size_type hull_count = 0U;

    for (size_type i = 0U; i < unique_count; ++i)
    {
        while (hull_count >= 2U && detail::orientation(
                   hull[hull_count - 2U], hull[hull_count - 1U], unique[i]) <= T{})
        {
            --hull_count;
        }
        hull[hull_count] = unique[i];
        ++hull_count;
    }

    CASTLE_CONST size_type lower_count = hull_count;
    for (size_type i = unique_count - 1U; i > 0U; --i)
    {
        CASTLE_CONST point2d<T>& point = unique[i - 1U];
        while (hull_count >= lower_count + 1U &&
               detail::orientation(
                   hull[hull_count - 2U], hull[hull_count - 1U], point) <= T{})
        {
            --hull_count;
        }
        hull[hull_count] = point;
        ++hull_count;
    }

    if (hull_count > 1U)
    {
        --hull_count;
    }

    for (size_type i = 0U; i < hull_count; ++i)
    {
        output.push_back(hull[i]);
    }
    return status::ok;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_ALGORITHMS_H
