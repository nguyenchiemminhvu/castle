// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-size 3D vector type and point/vector operators.
 *
 * Use vector3d for spatial displacements, directions, and vector products. The
 * type stores components inline, performs no allocation, and exposes floating-
 * point-only normalization and length helpers.
 */
#ifndef CASTLE_MATH_GEOMETRY_VECTOR3D_HPP
#define CASTLE_MATH_GEOMETRY_VECTOR3D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/geometry/point3d.hpp"
#include "castle/math/hypot.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Stores a 3D displacement or direction.
 *
 * @tparam T Arithmetic component type.
 */
template <typename T>
class vector3d
{
    static_assert(meta::is_arithmetic<T>::value, "vector3d requires an arithmetic type");

public:
    using value_type = T;

    /** @brief Constructs the zero vector. */
    CASTLE_CONSTEXPR vector3d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}), z_(T{}) {}

    /**
     * @brief Constructs a vector from explicit components.
     * @param x X component.
     * @param y Y component.
     * @param z Z component.
     */
    CASTLE_CONSTEXPR vector3d(T x, T y, T z) CASTLE_NOEXCEPT : x_(x), y_(y), z_(z) {}

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
     * @brief Returns the z component.
     * @return Stored z component.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T z() CASTLE_CONST CASTLE_NOEXCEPT { return z_; }

    /**
     * @brief Returns the squared Euclidean length.
     * @return x * x + y * y + z * z.
     * @note The result is not widened automatically.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T squared_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * x_ + y_ * y_ + z_ * z_);
    }

    /**
     * @brief Returns the Euclidean length.
     * @tparam U Floating-point type selected by default from T.
     * @return sqrt(x * x + y * y + z * z) computed via castle::math::hypot.
     * @note Available only when T is floating-point.
     */
    template <typename U = T>
    CASTLE_NODISCARD CASTLE_CONSTEXPR typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hypot(static_cast<U>(x_), static_cast<U>(y_), static_cast<U>(z_));
    }

    /**
     * @brief Reports whether the vector is exactly zero.
     * @return true when all components compare equal to zero.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool degenerate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == T{} && y_ == T{} && z_ == T{};
    }

    /**
     * @brief Computes the dot product.
     * @param other Vector to multiply with.
     * @return x * other.x + y * other.y + z * other.z.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * other.x_ + y_ * other.y_ + z_ * other.z_);
    }

    /**
     * @brief Computes the 3D cross product.
     * @param other Vector to multiply with.
     * @return Right-handed cross product of *this and other.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d cross(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(y_ * other.z_ - z_ * other.y_),
            static_cast<T>(z_ * other.x_ - x_ * other.z_),
            static_cast<T>(x_ * other.y_ - y_ * other.x_));
    }

    /**
     * @brief Returns a unit-length vector in the same direction.
     * @tparam U Floating-point type selected by default from T.
     * @return Normalized vector.
     * @note Available only when T is floating-point.
     * @warning Zero vectors trigger the Castle error handler assertion.
     */
    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, vector3d>::type
    normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST U len = length<U>();
        CASTLE_ASSERT(len > static_cast<U>(0), "cannot normalize a zero vector"); // LCOV_EXCL_BR_LINE
        return vector3d(
            static_cast<T>(x_ / len),
            static_cast<T>(y_ / len),
            static_cast<T>(z_ / len));
    }

    /**
     * @brief Returns the vector unchanged.
     * @return Copy of *this.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    /**
     * @brief Negates all components.
     * @return Vector (-x, -y, -z).
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(static_cast<T>(-x_), static_cast<T>(-y_), static_cast<T>(-z_));
    }

    /**
     * @brief Adds two vectors component-wise.
     * @param other Vector to add.
     * @return Sum of the two vectors.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator+(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(x_ + other.x_),
            static_cast<T>(y_ + other.y_),
            static_cast<T>(z_ + other.z_));
    }

    /**
     * @brief Subtracts two vectors component-wise.
     * @param other Vector to subtract.
     * @return Difference of the two vectors.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator-(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(x_ - other.x_),
            static_cast<T>(y_ - other.y_),
            static_cast<T>(z_ - other.z_));
    }

    /**
     * @brief Adds another vector in place.
     * @param other Vector to add.
     * @return Reference to *this.
     */
    CASTLE_CONSTEXPR vector3d& operator+=(CASTLE_CONST vector3d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ + other.x_);
        y_ = static_cast<T>(y_ + other.y_);
        z_ = static_cast<T>(z_ + other.z_);
        return *this;
    }

    /**
     * @brief Subtracts another vector in place.
     * @param other Vector to subtract.
     * @return Reference to *this.
     */
    CASTLE_CONSTEXPR vector3d& operator-=(CASTLE_CONST vector3d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ - other.x_);
        y_ = static_cast<T>(y_ - other.y_);
        z_ = static_cast<T>(z_ - other.z_);
        return *this;
    }

    /**
     * @brief Multiplies all components by a scalar.
     * @param scalar Scalar multiplier.
     * @return Scaled vector.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(x_ * scalar),
            static_cast<T>(y_ * scalar),
            static_cast<T>(z_ * scalar));
    }

    /**
     * @brief Divides all components by a scalar.
     * @param scalar Non-zero scalar divisor.
     * @return Scaled vector.
     * @warning scalar must not be zero.
     */
    CASTLE_NODISCARD vector3d operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), "vector division by zero"); // LCOV_EXCL_BR_LINE
        return vector3d(
            static_cast<T>(x_ / scalar),
            static_cast<T>(y_ / scalar),
            static_cast<T>(z_ / scalar));
    }

    /**
     * @brief Compares two vectors component-wise.
     * @param other Vector to compare with.
     * @return true when all components compare equal.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_ && z_ == other.z_; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Compares two vectors component-wise.
     * @param other Vector to compare with.
     * @return true when any component differs.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
    T z_;
};

/**
 * @brief Multiplies a vector by a scalar on the left.
 * @tparam T Arithmetic component type.
 * @param scalar Scalar multiplier.
 * @param value Vector to scale.
 * @return value * scalar.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d<T> operator*(T scalar, CASTLE_CONST vector3d<T>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

/**
 * @brief Computes the displacement from b to a.
 * @tparam T Arithmetic coordinate type.
 * @param a Head point.
 * @param b Tail point.
 * @return Vector a - b.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d<T> operator-(CASTLE_CONST point3d<T>& a, CASTLE_CONST point3d<T>& b) CASTLE_NOEXCEPT
{
    return vector3d<T>(
        static_cast<T>(a.x() - b.x()),
        static_cast<T>(a.y() - b.y()),
        static_cast<T>(a.z() - b.z()));
}

/**
 * @brief Translates a point by a vector.
 * @tparam T Arithmetic coordinate type.
 * @param point Point to translate.
 * @param value Translation vector.
 * @return point shifted by value.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point3d<T> operator+(CASTLE_CONST point3d<T>& point, CASTLE_CONST vector3d<T>& value) CASTLE_NOEXCEPT
{
    return point3d<T>(
        static_cast<T>(point.x() + value.x()),
        static_cast<T>(point.y() + value.y()),
        static_cast<T>(point.z() + value.z()));
}

/**
 * @brief Translates a point by the negated vector.
 * @tparam T Arithmetic coordinate type.
 * @param point Point to translate.
 * @param value Translation vector.
 * @return point shifted by -value.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point3d<T> operator-(CASTLE_CONST point3d<T>& point, CASTLE_CONST vector3d<T>& value) CASTLE_NOEXCEPT
{
    return point3d<T>(
        static_cast<T>(point.x() - value.x()),
        static_cast<T>(point.y() - value.y()),
        static_cast<T>(point.z() - value.z()));
}

/**
 * @brief Computes the 3D dot product.
 * @tparam T Arithmetic component type.
 * @param a First vector.
 * @param b Second vector.
 * @return a.dot(b).
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector3d<T>& a, CASTLE_CONST vector3d<T>& b) CASTLE_NOEXCEPT
{
    return a.dot(b);
}

/**
 * @brief Computes the 3D cross product.
 * @tparam T Arithmetic component type.
 * @param a First vector.
 * @param b Second vector.
 * @return a.cross(b).
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d<T> cross(CASTLE_CONST vector3d<T>& a, CASTLE_CONST vector3d<T>& b) CASTLE_NOEXCEPT
{
    return a.cross(b);
}

/**
 * @brief Computes the scalar triple product.
 * @tparam T Arithmetic component type.
 * @param a First vector.
 * @param b Second vector.
 * @param c Third vector.
 * @return a dot (b cross c).
 * @note The sign follows the right-handed orientation convention.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T scalar_triple_product(
    CASTLE_CONST vector3d<T>& a,
    CASTLE_CONST vector3d<T>& b,
    CASTLE_CONST vector3d<T>& c) CASTLE_NOEXCEPT
{
    return a.dot(b.cross(c));
}

}
}

#endif
