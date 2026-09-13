#ifndef CASTLE_MATH_GEOMETRY_CIRCLE3D_H
#define CASTLE_MATH_GEOMETRY_CIRCLE3D_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/geometry/detail.h"
#include "castle/math/geometry/point3d.h"
#include "castle/math/geometry/plane3d.h"
#include "castle/math/geometry/vector3d.h"

namespace castle
{
namespace math
{

// ============================================================================
// 3D circle represented by center + plane normal + non-negative radius.
//
// Real-life use cases: circular tool paths, planar sensor targets, mechanical
// rings, and intersections between spatial collision primitives.
// ============================================================================
template <typename T>
class circle3d
{
    static_assert(meta::is_floating_point<T>::value, "circle3d requires a floating-point type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR circle3d() CASTLE_NOEXCEPT
        : center_(), normal_(T{}, T{}, static_cast<T>(1)), radius_(T{})
    {
    }

    CASTLE_CONSTEXPR circle3d(
        CASTLE_CONST point3d<T>& center,
        CASTLE_CONST vector3d<T>& normal,
        T radius) CASTLE_NOEXCEPT
        : center_(center), normal_(normal), radius_(radius)
    {
        CASTLE_ASSERT(!normal_.degenerate(), "circle normal must be non-zero");
        CASTLE_ASSERT(radius_ >= T{}, "circle radius must be non-negative");
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point3d<T>& center() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return center_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST vector3d<T>& normal() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return normal_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T radius() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return radius_;
    }

    // ------------------------------------------------------------------------
    // Test membership in the filled circular disk.
    // ------------------------------------------------------------------------
    CASTLE_NODISCARD bool contains(CASTLE_CONST point3d<T>& point, T epsilon = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST vector3d<T> delta = point - center_;
        CASTLE_CONST T normal_length_sq = normal_.squared_length();
        CASTLE_CONST T plane_value = normal_.dot(delta);
        CASTLE_CONST T plane_limit = static_cast<T>(epsilon * epsilon * normal_length_sq);
        if (static_cast<T>(plane_value * plane_value) > plane_limit)
        {
            return false;
        }

        CASTLE_CONST T axial = static_cast<T>(delta.squared_length() -
                                       static_cast<T>((plane_value * plane_value) / normal_length_sq));
        if (axial < static_cast<T>(0))
        {
            return false;
        }
        CASTLE_CONST T distance = sqrt_real(axial);
        return distance <= static_cast<T>(radius_ + epsilon);
    }

    // ------------------------------------------------------------------------
    // Test membership on the circle circumference.
    // ------------------------------------------------------------------------
    CASTLE_NODISCARD bool on_circle(CASTLE_CONST point3d<T>& point, T epsilon = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST vector3d<T> delta = point - center_;
        CASTLE_CONST T normal_length_sq = normal_.squared_length();
        CASTLE_CONST T plane_value = normal_.dot(delta);
        CASTLE_CONST T plane_limit = static_cast<T>(epsilon * epsilon * normal_length_sq);
        if (static_cast<T>(plane_value * plane_value) > plane_limit)
        {
            return false;
        }

        CASTLE_CONST T axial = static_cast<T>(delta.squared_length() -
                                       static_cast<T>((plane_value * plane_value) / normal_length_sq));
        if (axial < static_cast<T>(0))
        {
            return false;
        }
        return abs(sqrt_real(axial) - radius_) <= static_cast<T>(epsilon);
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR plane3d<T> plane() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return plane3d<T>(center_, normal_);
    }

private:
    point3d<T> center_;
    vector3d<T> normal_;
    T radius_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_CIRCLE3D_H
