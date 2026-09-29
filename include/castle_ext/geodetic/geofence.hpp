// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file geofence.hpp
 * @brief Deterministic point-in-geofence containment tests for geographic coordinates.
 *
 * This header answers a single question: is a `geo_point<T>` inside a fence?
 * Three fence shapes are provided, matching the common embedded/automotive
 * geofencing use cases:
 *  - `circular_geofence<T, Ellipsoid>` / `point_in_circle()`: a center and a
 *    radius in meters, evaluated with the true great-circle (Haversine)
 *    distance so the check stays correct at every latitude.
 *  - `polygon_geofence<T, Capacity>`: a fixed-capacity, reusable, ordered
 *    boundary of vertices describing a region (e.g. a depot yard, a city, a
 *    country).
 *  - `point_in_boundary()`: the same boundary test as `polygon_geofence`, but
 *    against an ad-hoc, non-owned list of vertices (a raw pointer/count pair
 *    or a `castle::container::array_view`), for one-off checks against data
 *    that already lives elsewhere (e.g. a boundary table).
 *
 * @par Planar reduction of polygon boundaries
 * A geo_point boundary is not, strictly speaking, a polygon: the earth is not
 * flat and the edge between two consecutive vertices is not a straight line.
 * `to_plane_point()` maps `geo_point<T>` onto `castle::math::point2d<T>` with
 * an equirectangular (plate carree) projection -- longitude as `x`, latitude
 * as `y`, with no scaling. This projection is a monotonic, orientation
 * preserving bijection of the `(longitude, latitude)` domain, so the
 * ray-casting / Jordan Curve Theorem result (`inside` vs. `outside`) it
 * produces is exact for the polygon whose edges are straight lines *in that
 * projection*, which is the same simplification used by GeoJSON, Turf.js,
 * and most lightweight geofencing implementations. It is a topological
 * result, not a metrically exact one: edges are not geodesics, so containment
 * near a very long edge can disagree with a great-circle interpretation by
 * up to the edge's geodesic sag. Two preconditions follow from the
 * projection and are the caller's responsibility:
 *  - the boundary must not cross the +/-180 degrees antimeridian (shift the
 *    longitudes of such a boundary into a continuous range before use); and
 *  - the boundary should stay clear of the poles, where meridians converge
 *    and equirectangular distortion is largest.
 * A circular fence is deliberately **not** reduced to the plane: a circle in
 * `(longitude, latitude)` space is not a circle on the ellipsoid, so
 * `point_in_circle()` instead compares a true `haversine_distance()` against
 * the radius.
 *
 * @par Extending with new fence shapes
 * `to_plane_point()` is public so new boundary-shaped fences can reuse the
 * same projection together with `castle::math::point_in_polygon()`,
 * `castle::math::point_on_segment()`, or any other `castle::math::geometry`
 * algorithm. Any type exposing `bool contains(const geo_point<T>&, ...)`
 * (with defaulted trailing arguments) is automatically usable with
 * `is_inside()`, without modifying this header.
 *
 * @code
 * #include "castle_ext/geodetic/geofence.hpp"
 *
 * using castle::geodetic::geo_point;
 *
 * constexpr geo_point<double> hanoi(21.0278, 105.8342);
 *
 * // Circle: within 5 km of central Hanoi?
 * constexpr geo_point<double> depot(21.03, 105.85);
 * const bool near_depot = castle::geodetic::point_in_circle(hanoi, depot, 5000.0);
 *
 * // Region: reusable, fixed-capacity boundary.
 * castle::geodetic::polygon_geofence<double, 8U> province;
 * province.push_back({20.9, 105.5});
 * province.push_back({21.3, 105.5});
 * province.push_back({21.3, 106.1});
 * province.push_back({20.9, 106.1});
 * const bool in_province = province.contains(hanoi);
 *
 * // Ad-hoc array of boundary points, e.g. loaded from a table at runtime.
 * const geo_point<double> boundary[] = {{20.9, 105.5}, {21.3, 105.5}, {21.3, 106.1}, {20.9, 106.1}};
 * const bool in_boundary = castle::geodetic::point_in_boundary(hanoi, boundary, 4U);
 * @endcode
 */
#ifndef CASTLE_EXT_GEODETIC_GEOFENCE_HPP
#define CASTLE_EXT_GEODETIC_GEOFENCE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"
#include "castle/error/status.hpp"
#include "castle/math/geometry/geometry.hpp"
#include "castle_ext/geodetic/coord.hpp"
#include "castle_ext/geodetic/datum.hpp"
#include "castle_ext/geodetic/distance.hpp"

namespace castle
{
namespace geodetic
{

/**
 * @brief Projects a geographic point onto the plane using an equirectangular
 * (plate carree) projection.
 * @tparam T Floating-point coordinate type.
 * @param point Geographic point to project.
 * @return `castle::math::point2d<T>` with `x() == point.longitude()` and
 * `y() == point.latitude()`.
 * @note This projection applies no scaling: it exists to give boundary
 * checks a monotonic, orientation-preserving planar chart, not a
 * metrically accurate one. See the file-level documentation for the
 * antimeridian and pole preconditions this implies.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR castle::math::point2d<T> to_plane_point(CASTLE_CONST geo_point<T>& point) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "to_plane_point requires a floating-point type");
    return castle::math::point2d<T>(point.longitude(), point.latitude());
}

/// @name Circular geofences
/// @{

/**
 * @brief Tests whether a point lies within a circular geofence.
 * @tparam T Floating-point coordinate type.
 * @tparam Ellipsoid Ellipsoid policy used by the underlying great-circle
 * distance (see `datum.hpp`). Defaults to `wgs84_ellipsoid<T>`.
 * @param point Query point.
 * @param center Circle center.
 * @param radius_m Circle radius, in meters.
 * @param margin_m Extra tolerance added to `radius_m` before comparison; a
 * negative value shrinks the effective radius.
 * @return `true` when the great-circle distance from `center` to `point` is
 * at most `radius_m + margin_m`.
 * @warning `radius_m` must be non-negative.
 * @note Uses `haversine_distance()`, a spherical-earth approximation with up
 * to ~0.5% error. For sub-meter accuracy at large radii, compare against
 * `vincenty_inverse()` instead.
 */
template <typename T, typename Ellipsoid = wgs84_ellipsoid<T>>
CASTLE_NODISCARD bool point_in_circle(CASTLE_CONST geo_point<T>& point, CASTLE_CONST geo_point<T>& center,
                                       T radius_m, T margin_m = T{}) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "point_in_circle requires a floating-point type");
    CASTLE_ASSERT(radius_m >= T{}, // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::geodetic::point_in_circle: radius_m must be non-negative"));

    CASTLE_CONST T effective_radius = radius_m + margin_m;
    return haversine_distance<T, Ellipsoid>(center, point) <= effective_radius;
}

/**
 * @brief Stores a reusable circular geofence (center and radius).
 * @tparam T Floating-point coordinate type.
 * @tparam Ellipsoid Ellipsoid policy used by the underlying great-circle
 * distance (see `datum.hpp`). Defaults to `wgs84_ellipsoid<T>`.
 */
template <typename T, typename Ellipsoid = wgs84_ellipsoid<T>>
class circular_geofence
{
    static_assert(castle::meta::is_floating_point<T>::value, "circular_geofence requires a floating-point type");

public:
    using value_type = T;

    /** @brief Constructs a zero-radius fence centered at (0, 0). */
    CASTLE_CONSTEXPR circular_geofence() CASTLE_NOEXCEPT : center_(), radius_m_(T{}) {}

    /**
     * @brief Constructs a circular fence from a center and radius.
     * @param center Circle center.
     * @param radius_m Circle radius, in meters.
     * @warning `radius_m` must be non-negative.
     */
    CASTLE_CONSTEXPR circular_geofence(CASTLE_CONST geo_point<T>& center, T radius_m) CASTLE_NOEXCEPT
        : center_(center), radius_m_(radius_m)
    {
        CASTLE_ASSERT(radius_m_ >= T{}, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::geodetic::circular_geofence: radius_m must be non-negative"));
    }

    /** @brief Returns the fence center. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST geo_point<T>& center() CASTLE_CONST CASTLE_NOEXCEPT { return center_; }
    /** @brief Returns the fence radius, in meters. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T radius() CASTLE_CONST CASTLE_NOEXCEPT { return radius_m_; }

    /**
     * @brief Tests whether a point lies within this fence.
     * @param point Query point.
     * @param margin_m Extra tolerance added to the radius before comparison.
     * @return `true` when `point` is within `radius() + margin_m` of `center()`.
     */
    CASTLE_NODISCARD bool contains(CASTLE_CONST geo_point<T>& point, T margin_m = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return point_in_circle<T, Ellipsoid>(point, center_, radius_m_, margin_m);
    }

private:
    geo_point<T> center_;
    T radius_m_;
};

/// @}

/// @name Boundary (polygon) geofences
/// @{

/**
 * @brief Classifies a point against a boundary described by vertex memory.
 * @tparam T Floating-point coordinate type.
 * @param point Query point.
 * @param boundary Pointer to boundary vertices, in edge order (lat/lon).
 * @param count Number of vertices in `boundary`.
 * @param epsilon_deg Absolute tolerance, in decimal degrees, applied to the
 * boundary-edge test (see `castle::math::point_on_segment()`).
 * @return `true` when `point` is inside the boundary or on one of its edges.
 * @note This reproduces `castle::math::point_in_polygon()`'s ray-casting
 * algorithm directly over `to_plane_point()`-projected vertices, reusing
 * `castle::math::point_on_segment()` for the edge test. It intentionally does
 * not materialize a `castle::math::point2d<T>` buffer, so `count` is not
 * bounded by any fixed capacity -- useful for boundaries sourced from
 * external, arbitrarily sized tables (see `polygon_geofence` for a
 * fixed-capacity, owned alternative).
 * @warning Returns `false` when `boundary` is `nullptr` or `count < 3`,
 * matching `castle::math::point_in_polygon()`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool point_in_boundary(CASTLE_CONST geo_point<T>& point,
                                                          CASTLE_CONST geo_point<T>* boundary,
                                                          castle::size_type count,
                                                          T epsilon_deg = T{}) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "point_in_boundary requires a floating-point type");

    if (boundary == nullptr || count < 3U)
    {
        return false;
    }

    CASTLE_CONST castle::math::point2d<T> plane_point = to_plane_point(point);
    bool inside = false;
    castle::size_type previous = count - 1U;

    for (castle::size_type current = 0U; current < count; ++current)
    {
        CASTLE_CONST castle::math::point2d<T> a = to_plane_point(boundary[previous]);
        CASTLE_CONST castle::math::point2d<T> b = to_plane_point(boundary[current]);

        if (castle::math::point_on_segment(plane_point, a, b, epsilon_deg))
        {
            return true;
        }

        CASTLE_CONST bool crosses_scanline = ((a.y() > plane_point.y()) != (b.y() > plane_point.y()));
        if (crosses_scanline)
        {
            CASTLE_CONST T lhs = static_cast<T>(plane_point.x() - a.x()) * static_cast<T>(b.y() - a.y());
            CASTLE_CONST T rhs = static_cast<T>(plane_point.y() - a.y()) * static_cast<T>(b.x() - a.x());
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

/**
 * @brief Classifies a point against a boundary described by an array view.
 * @tparam T Floating-point coordinate type.
 * @param point Query point.
 * @param boundary Read-only view over boundary vertices, in edge order.
 * @param epsilon_deg Absolute tolerance, in decimal degrees, applied to the
 * boundary-edge test.
 * @return `true` when `point` is inside the boundary or on one of its edges.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool point_in_boundary(CASTLE_CONST geo_point<T>& point,
                                                          castle::container::array_view<CASTLE_CONST geo_point<T>> boundary,
                                                          T epsilon_deg = T{}) CASTLE_NOEXCEPT
{
    return point_in_boundary(point, boundary.data(), boundary.size(), epsilon_deg);
}

/**
 * @brief Read-only, non-owning view over a `geo_point<T>` boundary.
 * @tparam T Floating-point coordinate type.
 */
template <typename T>
using geo_boundary_view = castle::container::array_view<CASTLE_CONST geo_point<T>>;

/**
 * @brief Stores a reusable, fixed-capacity polygonal geofence (a region).
 * @tparam T Floating-point coordinate type.
 * @tparam Capacity Maximum number of stored boundary vertices.
 * @note The type does not enforce winding or closure; the first and last
 * vertices are implicitly connected, matching `castle::math::polygon2d`.
 */
template <typename T, castle::size_type Capacity>
class polygon_geofence
{
    static_assert(castle::meta::is_floating_point<T>::value, "polygon_geofence requires a floating-point type");
    static_assert(Capacity >= 3U, "polygon_geofence requires a capacity of at least 3 vertices to form a region");

public:
    using value_type = geo_point<T>;
    using container_type = castle::container::array<value_type, Capacity>;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;

    /** @brief Constructs an empty region. */
    CASTLE_CONSTEXPR polygon_geofence() CASTLE_NOEXCEPT : vertices_(), size_(0U) {}

    /** @brief Returns the current number of vertices. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR castle::size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    /** @brief Returns the maximum number of vertices. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR castle::size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return Capacity; }
    /** @brief Reports whether the region stores no vertices. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    /** @brief Reports whether the region reached its fixed capacity. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == Capacity; }

    /**
     * @brief Returns a read-only vertex reference.
     * @param index Vertex index in `[0, size())`.
     * @warning `index` must be smaller than `size()`.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST value_type& operator[](castle::size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < size_, CASTLE_ERROR_GENERIC("castle::geodetic::polygon_geofence: index out of range")); // LCOV_EXCL_BR_LINE
        return vertices_[index];
    }

    /**
     * @brief Returns a writable vertex reference.
     * @param index Vertex index in `[0, size())`.
     * @warning `index` must be smaller than `size()`.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR value_type& operator[](castle::size_type index) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < size_, CASTLE_ERROR_GENERIC("castle::geodetic::polygon_geofence: index out of range")); // LCOV_EXCL_BR_LINE
        return vertices_[index];
    }

    /**
     * @brief Appends a boundary vertex when capacity permits.
     * @param vertex Vertex to append, in edge order.
     * @return `castle::status::ok` on success or `castle::status::full` when
     * the region already stores `capacity()` vertices.
     */
    CASTLE_CONSTEXPR castle::status push_back(CASTLE_CONST value_type& vertex) CASTLE_NOEXCEPT
    {
        if (size_ >= Capacity)
        {
            return castle::status::full;
        }
        vertices_[size_] = vertex;
        ++size_;
        return castle::status::ok;
    }

    /**
     * @brief Removes the last boundary vertex.
     * @return `castle::status::ok` on success or `castle::status::empty` when
     * the region stores no vertices.
     */
    CASTLE_CONSTEXPR castle::status pop_back() CASTLE_NOEXCEPT
    {
        if (size_ == 0U)
        {
            return castle::status::empty;
        }
        --size_;
        return castle::status::ok;
    }

    /** @brief Removes all stored vertices. */
    CASTLE_CONSTEXPR void clear() CASTLE_NOEXCEPT { size_ = 0U; }

    /** @brief Returns the underlying contiguous storage pointer. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR value_type* data() CASTLE_NOEXCEPT { return vertices_.data(); }
    /** @brief Returns the underlying contiguous storage pointer. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST value_type* data() CASTLE_CONST CASTLE_NOEXCEPT { return vertices_.data(); }

    /** @brief Returns an iterator to the first stored vertex. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return vertices_.begin(); }
    /** @brief Returns a const iterator to the first stored vertex. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return vertices_.begin(); }
    /** @brief Returns an iterator one past the last stored vertex. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return vertices_.begin() + size_; }
    /** @brief Returns a const iterator one past the last stored vertex. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return vertices_.begin() + size_; }

    /**
     * @brief Tests whether a point lies within this region.
     * @param point Query point.
     * @param epsilon_deg Absolute tolerance, in decimal degrees, applied to
     * the boundary-edge test.
     * @return `true` when `point` is inside the region or on one of its edges;
     * `false` when fewer than 3 vertices are stored.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool contains(CASTLE_CONST value_type& point, T epsilon_deg = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return point_in_boundary(point, data(), size_, epsilon_deg);
    }

private:
    container_type vertices_;
    castle::size_type size_;
};

/**
 * @brief Classifies a point against a `polygon_geofence` region.
 * @tparam T Floating-point coordinate type.
 * @tparam Capacity Region capacity.
 * @param point Query point.
 * @param region Region whose active vertices are tested.
 * @param epsilon_deg Absolute tolerance, in decimal degrees, applied to the
 * boundary-edge test.
 * @return `true` when `point` is inside the region or on one of its edges.
 */
template <typename T, castle::size_type Capacity>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool point_in_boundary(CASTLE_CONST geo_point<T>& point,
                                                          CASTLE_CONST polygon_geofence<T, Capacity>& region,
                                                          T epsilon_deg = T{}) CASTLE_NOEXCEPT
{
    return region.contains(point, epsilon_deg);
}

/// @}

/**
 * @brief Generic containment dispatcher for any fence type.
 * @tparam T Floating-point coordinate type.
 * @tparam Fence Any type exposing `bool contains(const geo_point<T>&, ...)`
 * with defaulted trailing arguments, e.g. `circular_geofence` or
 * `polygon_geofence`.
 * @param point Query point.
 * @param fence Fence to test `point` against.
 * @return `fence.contains(point)`.
 * @note This overload lets user-defined fence shapes plug into the same call
 * site as the fences provided here, without modifying this header: any type
 * with a matching `contains()` member is accepted, and any mismatch is a
 * precompile (SFINAE) failure rather than a silent miscompile.
 */
template <typename T, typename Fence>
CASTLE_NODISCARD auto is_inside(CASTLE_CONST geo_point<T>& point, CASTLE_CONST Fence& fence) CASTLE_NOEXCEPT
    -> decltype(fence.contains(point))
{
    return fence.contains(point);
}

} // namespace geodetic
} // namespace castle

#endif // CASTLE_EXT_GEODETIC_GEOFENCE_HPP
