#ifndef CASTLE_MATH_GEOMETRY_POINT2D_H
#define CASTLE_MATH_GEOMETRY_POINT2D_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// 2D point.
//
// A point represents a position. It is deliberately kept separate from
// vector2d so that expressions such as point - point and point + vector remain
// dimensionally meaningful.
//
// Real-life use cases: screen/pixel coordinates, map positions, robot waypoints,
// CAD vertices, and sensor locations.
// ============================================================================
template <typename T>
class point2d
{
    static_assert(meta::is_arithmetic<T>::value, "point2d requires an arithmetic type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR point2d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}) {}

    CASTLE_CONSTEXPR point2d(T x, T y) CASTLE_NOEXCEPT : x_(x), y_(y) {}

    CASTLE_NODISCARD CASTLE_CONSTEXPR T x() CASTLE_CONST CASTLE_NOEXCEPT { return x_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T y() CASTLE_CONST CASTLE_NOEXCEPT { return y_; }

    CASTLE_CONSTEXPR void set_x(T value) CASTLE_NOEXCEPT { x_ = value; }
    CASTLE_CONSTEXPR void set_y(T value) CASTLE_NOEXCEPT { y_ = value; }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST point2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST point2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_POINT2D_H
