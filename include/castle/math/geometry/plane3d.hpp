// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Plane primitive for 3D half-space and incidence queries.
 *
 * Use plane3d when a calculation needs a point and an explicit normal rather
 * than an implicit equation. The stored normal is not normalized, which keeps
 * construction deterministic and avoids extra floating-point work.
 */
#ifndef CASTLE_MATH_GEOMETRY_PLANE3D_HPP
#define CASTLE_MATH_GEOMETRY_PLANE3D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/vector3d.hpp"
#include "castle/math/geometry/detail.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Represents a plane by one point and a non-zero normal.
 *
 * @tparam T Arithmetic coordinate type.
 * @note signed_value() returns the raw plane equation value, not Euclidean
 * distance, unless the normal is unit length.
 */
template <typename T>
class plane3d
{
    static_assert(meta::is_arithmetic<T>::value, "plane3d requires an arithmetic type");

public:
    using value_type = T;

    /** @brief Constructs a degenerate plane with zero normal. */
    CASTLE_CONSTEXPR plane3d() CASTLE_NOEXCEPT
        : point_(), normal_(T{}, T{}, T{})
    {
    }

    /**
     * @brief Constructs a plane from one point and a normal.
     * @param point Point lying on the plane.
     * @param normal Plane normal vector.
     * @warning normal must be non-zero.
     */
    CASTLE_CONSTEXPR plane3d(CASTLE_CONST point3d<T>& point, CASTLE_CONST vector3d<T>& normal) CASTLE_NOEXCEPT
        : point_(point), normal_(normal)
    {
        CASTLE_ASSERT(!normal_.degenerate(), "plane normal must be non-zero"); // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Returns the reference point stored on the plane.
     * @return Reference to the stored point.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point3d<T>& point() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return point_;
    }

    /**
     * @brief Returns the stored plane normal.
     * @return Reference to the normal vector.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST vector3d<T>& normal() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return normal_;
    }

    /**
     * @brief Evaluates the signed plane equation at a point.
     * @param value Point to classify.
     * @return normal dot (value - point()).
     * @note The magnitude scales with the normal length.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T signed_value(CASTLE_CONST point3d<T>& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return normal_.dot(value - point_);
    }

    /**
     * @brief Tests whether a point lies on the plane within a tolerance.
     * @param value Point to classify.
     * @param epsilon Absolute tolerance applied to signed_value().
     * @return true when abs(signed_value(value)) <= epsilon.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool contains(CASTLE_CONST point3d<T>& value, T epsilon = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return detail::near_zero(signed_value(value), epsilon);
    }

private:
    point3d<T> point_;
    vector3d<T> normal_;
};

}
}

#endif
