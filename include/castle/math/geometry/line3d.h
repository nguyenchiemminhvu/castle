#ifndef CASTLE_MATH_GEOMETRY_LINE3D_H
#define CASTLE_MATH_GEOMETRY_LINE3D_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/math/geometry/vector3d.h"

namespace castle
{
namespace math
{

template <typename T>
class line3d
{
    static_assert(meta::is_arithmetic<T>::value, "line3d requires an arithmetic type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR line3d() CASTLE_NOEXCEPT
        : origin_(), direction_(T{}, T{}, T{})
    {
    }

    CASTLE_CONSTEXPR line3d(CASTLE_CONST point3d<T>& origin, CASTLE_CONST vector3d<T>& direction) CASTLE_NOEXCEPT
        : origin_(origin), direction_(direction)
    {
    }

    CASTLE_CONSTEXPR line3d(CASTLE_CONST point3d<T>& a, CASTLE_CONST point3d<T>& b) CASTLE_NOEXCEPT
        : origin_(a), direction_(b - a)
    {
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point3d<T>& origin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return origin_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST vector3d<T>& direction() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return direction_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR point3d<T> point_at(T parameter) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return origin_ + direction_ * parameter;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool degenerate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return direction_.x() == T{} && direction_.y() == T{} && direction_.z() == T{};
    }

private:
    point3d<T> origin_;
    vector3d<T> direction_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_LINE3D_H
