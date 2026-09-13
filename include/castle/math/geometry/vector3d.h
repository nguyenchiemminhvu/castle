#ifndef CASTLE_MATH_GEOMETRY_VECTOR3D_H
#define CASTLE_MATH_GEOMETRY_VECTOR3D_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/geometry/point3d.h"
#include "castle/math/hypot.h"

namespace castle
{
namespace math
{

template <typename T>
class vector3d
{
    static_assert(meta::is_arithmetic<T>::value, "vector3d requires an arithmetic type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR vector3d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}), z_(T{}) {}

    CASTLE_CONSTEXPR vector3d(T x, T y, T z) CASTLE_NOEXCEPT : x_(x), y_(y), z_(z) {}

    CASTLE_NODISCARD CASTLE_CONSTEXPR T x() CASTLE_CONST CASTLE_NOEXCEPT { return x_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T y() CASTLE_CONST CASTLE_NOEXCEPT { return y_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T z() CASTLE_CONST CASTLE_NOEXCEPT { return z_; }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T squared_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * x_ + y_ * y_ + z_ * z_);
    }

    template <typename U = T>
    CASTLE_NODISCARD CASTLE_CONSTEXPR typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hypot(static_cast<U>(x_), static_cast<U>(y_), static_cast<U>(z_));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool degenerate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == T{} && y_ == T{} && z_ == T{};
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * other.x_ + y_ * other.y_ + z_ * other.z_);
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d cross(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(y_ * other.z_ - z_ * other.y_),
            static_cast<T>(z_ * other.x_ - x_ * other.z_),
            static_cast<T>(x_ * other.y_ - y_ * other.x_));
    }

    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, vector3d>::type
    normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST U len = length<U>();
        CASTLE_ASSERT(len > static_cast<U>(0), "cannot normalize a zero vector");
        return vector3d(
            static_cast<T>(x_ / len),
            static_cast<T>(y_ / len),
            static_cast<T>(z_ / len));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(static_cast<T>(-x_), static_cast<T>(-y_), static_cast<T>(-z_));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator+(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(x_ + other.x_),
            static_cast<T>(y_ + other.y_),
            static_cast<T>(z_ + other.z_));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator-(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(x_ - other.x_),
            static_cast<T>(y_ - other.y_),
            static_cast<T>(z_ - other.z_));
    }

    CASTLE_CONSTEXPR vector3d& operator+=(CASTLE_CONST vector3d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ + other.x_);
        y_ = static_cast<T>(y_ + other.y_);
        z_ = static_cast<T>(z_ + other.z_);
        return *this;
    }

    CASTLE_CONSTEXPR vector3d& operator-=(CASTLE_CONST vector3d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ - other.x_);
        y_ = static_cast<T>(y_ - other.y_);
        z_ = static_cast<T>(z_ - other.z_);
        return *this;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector3d(
            static_cast<T>(x_ * scalar),
            static_cast<T>(y_ * scalar),
            static_cast<T>(z_ * scalar));
    }

    CASTLE_NODISCARD vector3d operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), "vector division by zero");
        return vector3d(
            static_cast<T>(x_ / scalar),
            static_cast<T>(y_ / scalar),
            static_cast<T>(z_ / scalar));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_ && z_ == other.z_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST vector3d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
    T z_;
};

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d<T> operator*(T scalar, CASTLE_CONST vector3d<T>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d<T> operator-(CASTLE_CONST point3d<T>& a, CASTLE_CONST point3d<T>& b) CASTLE_NOEXCEPT
{
    return vector3d<T>(
        static_cast<T>(a.x() - b.x()),
        static_cast<T>(a.y() - b.y()),
        static_cast<T>(a.z() - b.z()));
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point3d<T> operator+(CASTLE_CONST point3d<T>& point, CASTLE_CONST vector3d<T>& value) CASTLE_NOEXCEPT
{
    return point3d<T>(
        static_cast<T>(point.x() + value.x()),
        static_cast<T>(point.y() + value.y()),
        static_cast<T>(point.z() + value.z()));
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point3d<T> operator-(CASTLE_CONST point3d<T>& point, CASTLE_CONST vector3d<T>& value) CASTLE_NOEXCEPT
{
    return point3d<T>(
        static_cast<T>(point.x() - value.x()),
        static_cast<T>(point.y() - value.y()),
        static_cast<T>(point.z() - value.z()));
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector3d<T>& a, CASTLE_CONST vector3d<T>& b) CASTLE_NOEXCEPT
{
    return a.dot(b);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector3d<T> cross(CASTLE_CONST vector3d<T>& a, CASTLE_CONST vector3d<T>& b) CASTLE_NOEXCEPT
{
    return a.cross(b);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T scalar_triple_product(
    CASTLE_CONST vector3d<T>& a,
    CASTLE_CONST vector3d<T>& b,
    CASTLE_CONST vector3d<T>& c) CASTLE_NOEXCEPT
{
    return a.dot(b.cross(c));
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_VECTOR3D_H
