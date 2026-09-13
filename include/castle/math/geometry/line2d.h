#ifndef CASTLE_MATH_GEOMETRY_LINE2D_H
#define CASTLE_MATH_GEOMETRY_LINE2D_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/math/geometry/vector2d.h"

namespace castle
{
namespace math
{

// ============================================================================
// Infinite 2D line represented by origin + direction.
//
// Parameterization: p(t) = origin + direction * t.
// ============================================================================
template <typename T>
class line2d
{
    static_assert(meta::is_arithmetic<T>::value, "line2d requires an arithmetic type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR line2d() CASTLE_NOEXCEPT
        : origin_(), direction_(T{}, T{})
    {
    }

    CASTLE_CONSTEXPR line2d(CASTLE_CONST point2d<T>& origin, CASTLE_CONST vector2d<T>& direction) CASTLE_NOEXCEPT
        : origin_(origin), direction_(direction)
    {
    }

    CASTLE_CONSTEXPR line2d(CASTLE_CONST point2d<T>& a, CASTLE_CONST point2d<T>& b) CASTLE_NOEXCEPT
        : origin_(a), direction_(b - a)
    {
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point2d<T>& origin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return origin_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST vector2d<T>& direction() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return direction_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR point2d<T> point_at(T parameter) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return origin_ + direction_ * parameter;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool degenerate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return direction_.x() == T{} && direction_.y() == T{};
    }

private:
    point2d<T> origin_;
    vector2d<T> direction_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_LINE2D_H
