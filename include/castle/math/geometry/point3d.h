#ifndef CASTLE_MATH_GEOMETRY_POINT3D_H
#define CASTLE_MATH_GEOMETRY_POINT3D_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// 3D point.
//
// Real-life use cases: robot pose coordinates, 3D sensor measurements, CAD
// vertices, and points in world coordinates.
// ============================================================================
template <typename T>
class point3d
{
    static_assert(meta::is_arithmetic<T>::value, "point3d requires an arithmetic type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR point3d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}), z_(T{}) {}

    CASTLE_CONSTEXPR point3d(T x, T y, T z) CASTLE_NOEXCEPT
        : x_(x), y_(y), z_(z)
    {
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T x() const CASTLE_NOEXCEPT { return x_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T y() const CASTLE_NOEXCEPT { return y_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T z() const CASTLE_NOEXCEPT { return z_; }

    CASTLE_CONSTEXPR void set_x(T value) CASTLE_NOEXCEPT { x_ = value; }
    CASTLE_CONSTEXPR void set_y(T value) CASTLE_NOEXCEPT { y_ = value; }
    CASTLE_CONSTEXPR void set_z(T value) CASTLE_NOEXCEPT { z_ = value; }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(const point3d& other) const CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_ && z_ == other.z_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(const point3d& other) const CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
    T z_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_POINT3D_H
