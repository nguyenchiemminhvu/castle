// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-size 2D point type.
 *
 * Use point2d to store positions in a plane without implying vector algebra.
 * The type keeps coordinates inline, performs no allocation, and relies only
 * on direct arithmetic comparisons supplied by the coordinate type.
 */
#ifndef CASTLE_MATH_GEOMETRY_POINT2D_HPP
#define CASTLE_MATH_GEOMETRY_POINT2D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Stores a position in 2D space.
 *
 * @tparam T Arithmetic coordinate type.
 * @note point2d is intentionally distinct from vector2d so point and vector
 * semantics stay explicit.
 */
template <typename T>
class point2d
{
    static_assert(meta::is_arithmetic<T>::value, "point2d requires an arithmetic type");

public:
    using value_type = T;

    /** @brief Constructs the point at the origin. */
    CASTLE_CONSTEXPR point2d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}) {}

    /**
     * @brief Constructs a point from explicit coordinates.
     * @param x X coordinate.
     * @param y Y coordinate.
     */
    CASTLE_CONSTEXPR point2d(T x, T y) CASTLE_NOEXCEPT : x_(x), y_(y) {}

    /**
     * @brief Returns the stored x coordinate.
     * @return The x coordinate.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T x() CASTLE_CONST CASTLE_NOEXCEPT { return x_; }

    /**
     * @brief Returns the stored y coordinate.
     * @return The y coordinate.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T y() CASTLE_CONST CASTLE_NOEXCEPT { return y_; }

    /**
     * @brief Replaces the x coordinate.
     * @param value New x coordinate.
     */
    CASTLE_CONSTEXPR void set_x(T value) CASTLE_NOEXCEPT { x_ = value; }

    /**
     * @brief Replaces the y coordinate.
     * @param value New y coordinate.
     */
    CASTLE_CONSTEXPR void set_y(T value) CASTLE_NOEXCEPT { y_ = value; }

    /**
     * @brief Compares two points component-wise.
     * @param other Point to compare with.
     * @return true when both coordinates compare equal.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST point2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Compares two points component-wise.
     * @param other Point to compare with.
     * @return true when any coordinate differs.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST point2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
};

}
}

#endif
