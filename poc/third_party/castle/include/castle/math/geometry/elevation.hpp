// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Elevation angle between Castle 3D points.
 *
 * Coordinates are interpreted in a local East-North-Up (ENU) frame: x points
 * East, y points North, and z points Up. Elevation is measured upward from the
 * local horizon (the xy plane): 0 = horizon, pi/2 = zenith, -pi/2 = nadir. It
 * is available in radians and in degrees. The helpers are header-only,
 * allocation-free, and exception-free; precision is that of the platform
 * atan2 implementation wrapped by castle/math/linalg/trigonometry.hpp.
 */
#ifndef CASTLE_MATH_GEOMETRY_ELEVATION_HPP
#define CASTLE_MATH_GEOMETRY_ELEVATION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/point3d.hpp"
#include "castle/math/hypot.hpp"
#include "castle/math/linalg/trigonometry.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes the elevation from one 3D point to another in radians.
 * @tparam T Floating-point coordinate type.
 * @param from Observer point.
 * @param to Target point.
 * @return atan2(dz, hypot(dx, dy)), the angle above the local horizon, in
 * [-pi/2, pi/2].
 * @note Returns zero when the points coincide; the guard avoids atan2(0, 0),
 * which is a domain error on some C libraries.
 */
template <typename T>
CASTLE_NODISCARD T elevation(CASTLE_CONST point3d<T>& from, CASTLE_CONST point3d<T>& to) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value, "elevation requires a floating-point type");
    CASTLE_CONST T up = static_cast<T>(to.z() - from.z());
    CASTLE_CONST T horizontal = hypot(static_cast<T>(to.x() - from.x()), static_cast<T>(to.y() - from.y()));
    if (horizontal == T{} && up == T{})
    {
        return T{};
    }
    return radians_atan2(up, horizontal);
}

/**
 * @brief Computes the elevation from one 3D point to another in degrees.
 * @tparam T Floating-point coordinate type.
 * @param from Observer point.
 * @param to Target point.
 * @return Angle above the local horizon, in [-90, 90].
 * @note Returns zero when the points coincide.
 */
template <typename T>
CASTLE_NODISCARD T elevation_degrees(CASTLE_CONST point3d<T>& from, CASTLE_CONST point3d<T>& to) CASTLE_NOEXCEPT
{
    return radians_to_degrees(elevation(from, to));
}

}
}

#endif
