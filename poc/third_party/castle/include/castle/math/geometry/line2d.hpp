// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Infinite 2D line represented by an origin and direction.
 *
 * Use line2d for deterministic parametric line calculations. The line stores
 * only a point and a direction vector, uses no dynamic memory, and does not
 * normalize its direction automatically.
 */
#ifndef CASTLE_MATH_GEOMETRY_LINE2D_HPP
#define CASTLE_MATH_GEOMETRY_LINE2D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/vector2d.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Represents an infinite line in 2D.
 *
 * @tparam T Arithmetic coordinate type.
 * @note The parameterization is p(t) = origin + direction * t.
 */
template <typename T>
class line2d
{
    static_assert(meta::is_arithmetic<T>::value, "line2d requires an arithmetic type");

public:
    using value_type = T;

    /** @brief Constructs a degenerate line at the origin. */
    CASTLE_CONSTEXPR line2d() CASTLE_NOEXCEPT
        : origin_(), direction_(T{}, T{})
    {
    }

    /**
     * @brief Constructs a line from an origin and direction.
     * @param origin Point on the line.
     * @param direction Direction vector used by the parameterization.
     * @note The direction is stored as provided and is not normalized.
     */
    CASTLE_CONSTEXPR line2d(CASTLE_CONST point2d<T>& origin, CASTLE_CONST vector2d<T>& direction) CASTLE_NOEXCEPT
        : origin_(origin), direction_(direction)
    {
    }

    /**
     * @brief Constructs the line through two points.
     * @param a First point.
     * @param b Second point.
     * @note The stored direction is b - a and may be zero when the points are equal.
     */
    CASTLE_CONSTEXPR line2d(CASTLE_CONST point2d<T>& a, CASTLE_CONST point2d<T>& b) CASTLE_NOEXCEPT
        : origin_(a), direction_(b - a)
    {
    }

    /**
     * @brief Returns the line origin.
     * @return Reference to the stored origin point.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point2d<T>& origin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return origin_;
    }

    /**
     * @brief Returns the line direction.
     * @return Reference to the stored direction vector.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST vector2d<T>& direction() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return direction_;
    }

    /**
     * @brief Evaluates the line parameterization.
     * @param parameter Scalar parameter t in p(t) = origin + direction * t.
     * @return Point on the line at the requested parameter.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR point2d<T> point_at(T parameter) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return origin_ + direction_ * parameter;
    }

    /**
     * @brief Reports whether the direction vector is exactly zero.
     * @return true when both direction components are zero.
     * @warning No epsilon tolerance is applied.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool degenerate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return direction_.x() == T{} && direction_.y() == T{}; // LCOV_EXCL_BR_LINE
    }

private:
    point2d<T> origin_;
    vector2d<T> direction_;
};

}
}

#endif
