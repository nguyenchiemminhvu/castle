#ifndef CASTLE_MATH_GEOMETRY_CIRCLE2D_H
#define CASTLE_MATH_GEOMETRY_CIRCLE2D_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/geometry/point2d.h"
#include "castle/math/geometry/vector2d.h"

namespace castle
{
namespace math
{

// ============================================================================
// 2D circle represented by center + non-negative radius.
//
// Real-life use cases: proximity sensors, 2D collision envelopes, range rings,
// and circular zones on maps.
// ============================================================================
template <typename T>
class circle2d
{
    static_assert(meta::is_floating_point<T>::value, "circle2d requires a floating-point type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR circle2d() CASTLE_NOEXCEPT
        : center_(), radius_(T{})
    {
    }

    CASTLE_CONSTEXPR circle2d(CASTLE_CONST point2d<T>& center, T radius) CASTLE_NOEXCEPT
        : center_(center), radius_(radius)
    {
        CASTLE_ASSERT(radius_ >= T{}, "circle radius must be non-negative");
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point2d<T>& center() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return center_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T radius() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return radius_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool contains(CASTLE_CONST point2d<T>& point, T epsilon = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST vector2d<T> delta = point - center_;
        CASTLE_CONST T expanded_radius = static_cast<T>(radius_ + epsilon);
        return delta.squared_length() <= static_cast<T>(expanded_radius * expanded_radius);
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool on_circle(CASTLE_CONST point2d<T>& point, T epsilon = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST vector2d<T> delta = point - center_;
        CASTLE_CONST T distance_squared = delta.squared_length();
        CASTLE_CONST T outer_radius = static_cast<T>(radius_ + epsilon);
        CASTLE_CONST T inner_radius = (radius_ > epsilon)
                                   ? static_cast<T>(radius_ - epsilon)
                                   : T{};
        return distance_squared <= static_cast<T>(outer_radius * outer_radius) &&
               distance_squared >= static_cast<T>(inner_radius * inner_radius);
    }

private:
    point2d<T> center_;
    T radius_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_CIRCLE2D_H
