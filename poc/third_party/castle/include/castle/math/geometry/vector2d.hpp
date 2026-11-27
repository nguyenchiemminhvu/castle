// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-size 2D vector type and point/vector operators.
 *
 * Use vector2d for displacements, directions, and planar products such as dot
 * and signed cross. The implementation stays allocation-free and deterministic,
 * with floating-point-only normalization and length queries.
 */
#ifndef CASTLE_MATH_GEOMETRY_VECTOR2D_HPP
#define CASTLE_MATH_GEOMETRY_VECTOR2D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/point2d.hpp"
#include "castle/math/square.hpp"
#include "castle/math/hypot.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Stores a 2D displacement or direction.
 *
 * @tparam T Arithmetic component type.
 */
template <typename T>
class vector2d
{
    static_assert(meta::is_arithmetic<T>::value, "vector2d requires an arithmetic type");

public:
    using value_type = T;

    /** @brief Constructs the zero vector. */
    CASTLE_CONSTEXPR vector2d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}) {}

    /**
     * @brief Constructs a vector from explicit components.
     * @param x X component.
     * @param y Y component.
     */
    CASTLE_CONSTEXPR vector2d(T x, T y) CASTLE_NOEXCEPT : x_(x), y_(y) {}

    /**
     * @brief Returns the x component.
     * @return Stored x component.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T x() CASTLE_CONST CASTLE_NOEXCEPT { return x_; }

    /**
     * @brief Returns the y component.
     * @return Stored y component.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T y() CASTLE_CONST CASTLE_NOEXCEPT { return y_; }

    /**
     * @brief Returns the squared Euclidean length.
     * @return x * x + y * y.
     * @note The result is not widened automatically.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T squared_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * x_ + y_ * y_);
    }

    /**
     * @brief Returns the Euclidean length.
     * @tparam U Floating-point type selected by default from T.
     * @return sqrt(x * x + y * y) computed via castle::math::hypot.
     * @note Available only when T is floating-point.
     */
    template <typename U = T>
    CASTLE_NODISCARD CASTLE_CONSTEXPR typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hypot(static_cast<U>(x_), static_cast<U>(y_));
    }

    /**
     * @brief Computes the dot product.
     * @param other Vector to multiply with.
     * @return x * other.x + y * other.y.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * other.x_ + y_ * other.y_);
    }

    /**
     * @brief Returns the counter-clockwise perpendicular vector.
     * @return Vector (-y, x).
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d perpendicular() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(-y_), x_);
    }

    /**
     * @brief Computes the signed 2D cross product.
     * @param other Vector to multiply with.
     * @return x * other.y - y * other.x.
     * @note This is the z component of the 3D cross product for XY-plane vectors.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T cross(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * other.y_ - y_ * other.x_);
    }

    /**
     * @brief Returns a unit-length vector in the same direction.
     * @tparam U Floating-point type selected by default from T.
     * @return Normalized vector.
     * @note Available only when T is floating-point.
     * @warning Zero vectors trigger the Castle error handler assertion.
     */
    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, vector2d>::type
    normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST U len = length<U>();
        CASTLE_ASSERT(len > static_cast<U>(0), "cannot normalize a zero vector"); // LCOV_EXCL_BR_LINE
        return vector2d(
            static_cast<T>(x_ / len),
            static_cast<T>(y_ / len));
    }

    /**
     * @brief Returns the vector unchanged.
     * @return Copy of *this.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    /**
     * @brief Negates both components.
     * @return Vector (-x, -y).
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(-x_), static_cast<T>(-y_));
    }

    /**
     * @brief Adds two vectors component-wise.
     * @param other Vector to add.
     * @return Sum of the two vectors.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator+(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(x_ + other.x_), static_cast<T>(y_ + other.y_));
    }

    /**
     * @brief Subtracts two vectors component-wise.
     * @param other Vector to subtract.
     * @return Difference of the two vectors.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator-(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(x_ - other.x_), static_cast<T>(y_ - other.y_));
    }

    /**
     * @brief Adds another vector in place.
     * @param other Vector to add.
     * @return Reference to *this.
     */
    CASTLE_CONSTEXPR vector2d& operator+=(CASTLE_CONST vector2d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ + other.x_);
        y_ = static_cast<T>(y_ + other.y_);
        return *this;
    }

    /**
     * @brief Subtracts another vector in place.
     * @param other Vector to subtract.
     * @return Reference to *this.
     */
    CASTLE_CONSTEXPR vector2d& operator-=(CASTLE_CONST vector2d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ - other.x_);
        y_ = static_cast<T>(y_ - other.y_);
        return *this;
    }

    /**
     * @brief Multiplies both components by a scalar.
     * @param scalar Scalar multiplier.
     * @return Scaled vector.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(x_ * scalar), static_cast<T>(y_ * scalar));
    }

    /**
     * @brief Divides both components by a scalar.
     * @param scalar Non-zero scalar divisor.
     * @return Scaled vector.
     * @warning scalar must not be zero.
     */
    CASTLE_NODISCARD vector2d operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), "vector division by zero"); // LCOV_EXCL_BR_LINE
        return vector2d(static_cast<T>(x_ / scalar), static_cast<T>(y_ / scalar));
    }

    /**
     * @brief Compares two vectors component-wise.
     * @param other Vector to compare with.
     * @return true when both components compare equal.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Compares two vectors component-wise.
     * @param other Vector to compare with.
     * @return true when any component differs.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
};

/**
 * @brief Multiplies a vector by a scalar on the left.
 * @tparam T Arithmetic component type.
 * @param scalar Scalar multiplier.
 * @param value Vector to scale.
 * @return value * scalar.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d<T> operator*(T scalar, CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

/**
 * @brief Computes the 2D dot product.
 * @tparam T Arithmetic component type.
 * @param first First vector.
 * @param second Second vector.
 * @return first.dot(second).
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector2d<T>& first, CASTLE_CONST vector2d<T>& second) CASTLE_NOEXCEPT
{
    return first.dot(second);
}

/**
 * @brief Computes the signed 2D cross product.
 * @tparam T Arithmetic component type.
 * @param first First vector.
 * @param second Second vector.
 * @return first.cross(second).
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross(CASTLE_CONST vector2d<T>& first, CASTLE_CONST vector2d<T>& second) CASTLE_NOEXCEPT
{
    return first.cross(second);
}

/**
 * @brief Computes the displacement from b to a.
 * @tparam T Arithmetic coordinate type.
 * @param a Head point.
 * @param b Tail point.
 * @return Vector a - b.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d<T> operator-(CASTLE_CONST point2d<T>& a, CASTLE_CONST point2d<T>& b) CASTLE_NOEXCEPT
{
    return vector2d<T>(static_cast<T>(a.x() - b.x()), static_cast<T>(a.y() - b.y()));
}

/**
 * @brief Translates a point by a vector.
 * @tparam T Arithmetic coordinate type.
 * @param point Point to translate.
 * @param value Translation vector.
 * @return point shifted by value.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point2d<T> operator+(CASTLE_CONST point2d<T>& point, CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    return point2d<T>(
        static_cast<T>(point.x() + value.x()),
        static_cast<T>(point.y() + value.y()));
}

/**
 * @brief Translates a point by the negated vector.
 * @tparam T Arithmetic coordinate type.
 * @param point Point to translate.
 * @param value Translation vector.
 * @return point shifted by -value.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point2d<T> operator-(CASTLE_CONST point2d<T>& point, CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    return point2d<T>(
        static_cast<T>(point.x() - value.x()),
        static_cast<T>(point.y() - value.y()));
}

}
}

#endif
