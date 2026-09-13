#ifndef CASTLE_MATH_GEOMETRY_PLANE3D_H
#define CASTLE_MATH_GEOMETRY_PLANE3D_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/geometry/vector3d.h"
#include "castle/math/geometry/detail.h"

namespace castle
{
namespace math
{

// ============================================================================
// 3D plane represented by one point and a non-zero normal.
//
// Real-life use cases: collision surfaces, robot/world frames, clipping planes,
// ray casting, and fitting geometry to sensor data.
// ============================================================================
template <typename T>
class plane3d
{
    static_assert(meta::is_arithmetic<T>::value, "plane3d requires an arithmetic type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR plane3d() CASTLE_NOEXCEPT
        : point_(), normal_(T{}, T{}, T{})
    {
    }

    CASTLE_CONSTEXPR plane3d(const point3d<T>& point, const vector3d<T>& normal) CASTLE_NOEXCEPT
        : point_(point), normal_(normal)
    {
        CASTLE_ASSERT(!normal_.degenerate(), "plane normal must be non-zero");
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR const point3d<T>& point() const CASTLE_NOEXCEPT
    {
        return point_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR const vector3d<T>& normal() const CASTLE_NOEXCEPT
    {
        return normal_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T signed_value(const point3d<T>& value) const CASTLE_NOEXCEPT
    {
        return normal_.dot(value - point_);
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool contains(const point3d<T>& value, T epsilon = T{}) const CASTLE_NOEXCEPT
    {
        return detail::near_zero(signed_value(value), epsilon);
    }

private:
    point3d<T> point_;
    vector3d<T> normal_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_PLANE3D_H
