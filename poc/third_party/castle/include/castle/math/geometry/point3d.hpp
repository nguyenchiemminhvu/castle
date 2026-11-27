// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-size 3D point type.
 *
 * Use point3d to store positions in 3D space without conflating locations and
 * directions. Coordinates are embedded directly in the object, with no heap
 * allocation, exceptions, RTTI, or STL dependencies.
 */
#ifndef CASTLE_MATH_GEOMETRY_POINT3D_HPP
#define CASTLE_MATH_GEOMETRY_POINT3D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Stores a position in 3D space.
 *
 * @tparam T Arithmetic coordinate type.
 * @note point3d is separate from vector3d so displacement and position remain
 * distinct at the type level.
 */
template <typename T>
class point3d
{
    static_assert(meta::is_arithmetic<T>::value, "point3d requires an arithmetic type");

public:
    using value_type = T;

    /** @brief Constructs the point at the origin. */
    CASTLE_CONSTEXPR point3d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}), z_(T{}) {}

    /**
     * @brief Constructs a point from explicit coordinates.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param z Z coordinate.
     */
    CASTLE_CONSTEXPR point3d(T x, T y, T z) CASTLE_NOEXCEPT
        : x_(x), y_(y), z_(z)
    {
    }

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
     * @brief Returns the stored z coordinate.
     * @return The z coordinate.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T z() CASTLE_CONST CASTLE_NOEXCEPT { return z_; }

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
     * @brief Replaces the z coordinate.
     * @param value New z coordinate.
     */
    CASTLE_CONSTEXPR void set_z(T value) CASTLE_NOEXCEPT { z_ = value; }

    /**
     * @brief Compares two points component-wise.
     * @param other Point to compare with.
     * @return true when all coordinates compare equal.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST point3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_ && z_ == other.z_; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Compares two points component-wise.
     * @param other Point to compare with.
     * @return true when any coordinate differs.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST point3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
    T z_;
};

}
}

#endif
