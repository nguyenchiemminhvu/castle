// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Circle primitive embedded in 3D space.
 *
 * Use circle3d for planar disks and circumferences with an explicit normal.
 * The circle stores a center, an arbitrary non-zero normal, and a radius, all
 * inline and without any hidden allocation or normalization step.
 */
#ifndef CASTLE_MATH_GEOMETRY_CIRCLE3D_HPP
#define CASTLE_MATH_GEOMETRY_CIRCLE3D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/detail.hpp"
#include "castle/math/geometry/point3d.hpp"
#include "castle/math/geometry/plane3d.hpp"
#include "castle/math/geometry/vector3d.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Represents a circle in 3D by center, normal, and radius.
 *
 * @tparam T Floating-point coordinate type.
 * @note The normal does not need to be unit length.
 */
template <typename T>
class circle3d
{
    static_assert(meta::is_floating_point<T>::value, "circle3d requires a floating-point type");

public:
    using value_type = T;

    /** @brief Constructs a zero-radius circle in the XY plane. */
    CASTLE_CONSTEXPR circle3d() CASTLE_NOEXCEPT
        : center_(), normal_(T{}, T{}, static_cast<T>(1)), radius_(T{})
    {
    }

    /**
     * @brief Constructs a circle from center, normal, and radius.
     * @param center Circle center.
     * @param normal Circle plane normal.
     * @param radius Circle radius.
     * @warning normal must be non-zero and radius must be non-negative.
     */
    CASTLE_CONSTEXPR circle3d(
        CASTLE_CONST point3d<T>& center,
        CASTLE_CONST vector3d<T>& normal,
        T radius) CASTLE_NOEXCEPT
        : center_(center), normal_(normal), radius_(radius)
    {
        CASTLE_ASSERT(!normal_.degenerate(), "circle normal must be non-zero"); // LCOV_EXCL_BR_LINE
        CASTLE_ASSERT(radius_ >= T{}, "circle radius must be non-negative"); // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Returns the circle center.
     * @return Reference to the stored center point.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point3d<T>& center() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return center_;
    }

    /**
     * @brief Returns the circle normal.
     * @return Reference to the stored normal vector.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST vector3d<T>& normal() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return normal_;
    }

    /**
     * @brief Returns the circle radius.
     * @return Non-negative radius value.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T radius() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return radius_;
    }

    /**
     * @brief Tests whether a point lies in the closed circular disk.
     * @param point Point to test.
     * @param epsilon Radial and plane tolerance.
     * @return true when the point is close enough to the plane and within the radius.
     * @note Plane tolerance is scaled by the squared normal length because the
     * normal is not normalized.
     */
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

    /**
     * @brief Tests whether a point lies on the circumference.
     * @param point Point to test.
     * @param epsilon Radial and plane tolerance.
     * @return true when the point lies on the circle within epsilon.
     * @note The point must satisfy both the plane test and the radial test.
     */
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

    /**
     * @brief Returns the supporting plane of the circle.
     * @return plane3d constructed from center() and normal().
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR plane3d<T> plane() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return plane3d<T>(center_, normal_);
    }

private:
    point3d<T> center_;
    vector3d<T> normal_;
    T radius_;
};

}
}

#endif
