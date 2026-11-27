// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Circle primitive for 2D containment tests.
 *
 * Use circle2d for radius checks, simple collision envelopes, and planar
 * geometry queries. The circle stores a center and non-negative radius only,
 * performs no allocation, and uses exact or epsilon-based comparisons.
 */
#ifndef CASTLE_MATH_GEOMETRY_CIRCLE2D_HPP
#define CASTLE_MATH_GEOMETRY_CIRCLE2D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/point2d.hpp"
#include "castle/math/geometry/vector2d.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Represents a 2D circle by center and radius.
 *
 * @tparam T Floating-point coordinate type.
 * @note The radius is asserted to be non-negative.
 */
template <typename T>
class circle2d
{
    static_assert(meta::is_floating_point<T>::value, "circle2d requires a floating-point type");

public:
    using value_type = T;

    /** @brief Constructs a zero-radius circle at the origin. */
    CASTLE_CONSTEXPR circle2d() CASTLE_NOEXCEPT
        : center_(), radius_(T{})
    {
    }

    /**
     * @brief Constructs a circle from center and radius.
     * @param center Circle center.
     * @param radius Circle radius.
     * @warning radius must be non-negative.
     */
    CASTLE_CONSTEXPR circle2d(CASTLE_CONST point2d<T>& center, T radius) CASTLE_NOEXCEPT
        : center_(center), radius_(radius)
    {
        CASTLE_ASSERT(radius_ >= T{}, "circle radius must be non-negative"); // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Returns the circle center.
     * @return Reference to the stored center point.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST point2d<T>& center() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return center_;
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
     * @brief Tests whether a point lies in the closed disk.
     * @param point Point to test.
     * @param epsilon Extra radial tolerance added to the radius.
     * @return true when the point is inside or on the expanded disk.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool contains(CASTLE_CONST point2d<T>& point, T epsilon = T{}) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST vector2d<T> delta = point - center_;
        CASTLE_CONST T expanded_radius = static_cast<T>(radius_ + epsilon);
        return delta.squared_length() <= static_cast<T>(expanded_radius * expanded_radius);
    }

    /**
     * @brief Tests whether a point lies on the circumference band.
     * @param point Point to test.
     * @param epsilon Symmetric radial tolerance around the circle radius.
     * @return true when the point falls between radius - epsilon and radius + epsilon.
     * @note The inner radius is clamped to zero when epsilon exceeds the radius.
     */
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

}
}

#endif
