// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Azimuth angle between Castle 2D and 3D points.
 *
 * Coordinates are interpreted in a local East-North-Up (ENU) frame: x points
 * East, y points North, and z points Up. Azimuth is measured clockwise from
 * North (0 = North, pi/2 = East, pi = South, 3*pi/2 = West) and is available
 * in radians and in degrees. The helpers are header-only, allocation-free, and
 * exception-free; precision is that of the platform atan2 implementation
 * wrapped by castle/math/linalg/trigonometry.hpp.
 */
#ifndef CASTLE_MATH_GEOMETRY_AZIMUTH_HPP
#define CASTLE_MATH_GEOMETRY_AZIMUTH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/point2d.hpp"
#include "castle/math/geometry/point3d.hpp"
#include "castle/math/linalg/trigonometry.hpp"

namespace castle
{
namespace math
{
namespace detail
{

/**
 * @brief Wraps an angle from [-full_turn / 2, full_turn / 2] into [0, full_turn).
 * @tparam T Floating-point angle type.
 * @param angle Angle to wrap, as returned by atan2 in the matching unit.
 * @param full_turn One full revolution in the same unit (tau or 360).
 * @return Equivalent angle in [0, full_turn).
 * @note A tiny negative angle can round to exactly full_turn after the
 * addition; that case is folded back to zero so the upper bound stays open.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T wrap_azimuth(T angle, T full_turn) CASTLE_NOEXCEPT
{
    if (angle < T{})
    {
        angle = static_cast<T>(angle + full_turn);
    }
    if (angle >= full_turn)
    {
        angle = T{};
    }
    return angle;
}

/**
 * @brief Computes the signed azimuth of a horizontal displacement.
 * @tparam T Floating-point component type.
 * @param east East (x) displacement.
 * @param north North (y) displacement.
 * @return atan2(east, north) in radians within [-pi, pi], or zero when both
 * components are zero.
 * @note The zero guard avoids atan2(0, 0), which is a domain error on some C
 * libraries and yields -pi for negative zeros on IEEE platforms.
 */
template <typename T>
CASTLE_NODISCARD T signed_azimuth(T east, T north) CASTLE_NOEXCEPT
{
    if (east == T{} && north == T{})
    {
        return T{};
    }
    return radians_atan2(east, north);
}

}

/**
 * @brief Computes the azimuth from one 2D point to another in radians.
 * @tparam T Floating-point coordinate type.
 * @param from Observer point.
 * @param to Target point.
 * @return Clockwise angle from North (+y) toward East (+x), in [0, 2*pi).
 * @note Returns zero when the points coincide.
 */
template <typename T>
CASTLE_NODISCARD T azimuth(CASTLE_CONST point2d<T>& from, CASTLE_CONST point2d<T>& to) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value, "azimuth requires a floating-point type");
    return detail::wrap_azimuth(
        detail::signed_azimuth(static_cast<T>(to.x() - from.x()), static_cast<T>(to.y() - from.y())),
        tau<T>());
}

/**
 * @brief Computes the azimuth from one 2D point to another in degrees.
 * @tparam T Floating-point coordinate type.
 * @param from Observer point.
 * @param to Target point.
 * @return Clockwise angle from North (+y) toward East (+x), in [0, 360).
 * @note Returns zero when the points coincide.
 */
template <typename T>
CASTLE_NODISCARD T azimuth_degrees(CASTLE_CONST point2d<T>& from, CASTLE_CONST point2d<T>& to) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value, "azimuth_degrees requires a floating-point type");
    return detail::wrap_azimuth(
        radians_to_degrees(detail::signed_azimuth(static_cast<T>(to.x() - from.x()),
                                                  static_cast<T>(to.y() - from.y()))),
        static_cast<T>(360));
}

/**
 * @brief Computes the azimuth from one 3D point to another in radians.
 * @tparam T Floating-point coordinate type.
 * @param from Observer point.
 * @param to Target point.
 * @return Clockwise angle from North (+y) toward East (+x), in [0, 2*pi).
 * @note The z displacement does not affect the result. Returns zero when the
 * target lies straight above or below the observer, or the points coincide.
 */
template <typename T>
CASTLE_NODISCARD T azimuth(CASTLE_CONST point3d<T>& from, CASTLE_CONST point3d<T>& to) CASTLE_NOEXCEPT
{
    return azimuth(point2d<T>(from.x(), from.y()), point2d<T>(to.x(), to.y()));
}

/**
 * @brief Computes the azimuth from one 3D point to another in degrees.
 * @tparam T Floating-point coordinate type.
 * @param from Observer point.
 * @param to Target point.
 * @return Clockwise angle from North (+y) toward East (+x), in [0, 360).
 * @note The z displacement does not affect the result. Returns zero when the
 * target lies straight above or below the observer, or the points coincide.
 */
template <typename T>
CASTLE_NODISCARD T azimuth_degrees(CASTLE_CONST point3d<T>& from, CASTLE_CONST point3d<T>& to) CASTLE_NOEXCEPT
{
    return azimuth_degrees(point2d<T>(from.x(), from.y()), point2d<T>(to.x(), to.y()));
}

}
}

#endif
