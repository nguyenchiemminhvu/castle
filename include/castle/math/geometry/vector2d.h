#ifndef CASTLE_MATH_GEOMETRY_VECTOR2D_H
#define CASTLE_MATH_GEOMETRY_VECTOR2D_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/geometry/point2d.h"
#include "castle/math/square.h"
#include "castle/math/hypot.h"

namespace castle
{
namespace math
{

template <typename T>
class vector2d
{
    static_assert(meta::is_arithmetic<T>::value, "vector2d requires an arithmetic type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR vector2d() CASTLE_NOEXCEPT : x_(T{}), y_(T{}) {}

    CASTLE_CONSTEXPR vector2d(T x, T y) CASTLE_NOEXCEPT : x_(x), y_(y) {}

    CASTLE_NODISCARD CASTLE_CONSTEXPR T x() CASTLE_CONST CASTLE_NOEXCEPT { return x_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T y() CASTLE_CONST CASTLE_NOEXCEPT { return y_; }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T squared_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * x_ + y_ * y_);
    }

    template <typename U = T>
    CASTLE_NODISCARD CASTLE_CONSTEXPR typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hypot(static_cast<U>(x_), static_cast<U>(y_));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * other.x_ + y_ * other.y_);
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d perpendicular() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(-y_), x_);
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T cross(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(x_ * other.y_ - y_ * other.x_);
    }

    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, vector2d>::type
    normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST U len = length<U>();
        CASTLE_ASSERT(len > static_cast<U>(0), "cannot normalize a zero vector");
        return vector2d(
            static_cast<T>(x_ / len),
            static_cast<T>(y_ / len));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(-x_), static_cast<T>(-y_));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator+(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(x_ + other.x_), static_cast<T>(y_ + other.y_));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator-(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(x_ - other.x_), static_cast<T>(y_ - other.y_));
    }

    CASTLE_CONSTEXPR vector2d& operator+=(CASTLE_CONST vector2d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ + other.x_);
        y_ = static_cast<T>(y_ + other.y_);
        return *this;
    }

    CASTLE_CONSTEXPR vector2d& operator-=(CASTLE_CONST vector2d& other) CASTLE_NOEXCEPT
    {
        x_ = static_cast<T>(x_ - other.x_);
        y_ = static_cast<T>(y_ - other.y_);
        return *this;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return vector2d(static_cast<T>(x_ * scalar), static_cast<T>(y_ * scalar));
    }

    CASTLE_NODISCARD vector2d operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), "vector division by zero");
        return vector2d(static_cast<T>(x_ / scalar), static_cast<T>(y_ / scalar));
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return x_ == other.x_ && y_ == other.y_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST vector2d& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T x_;
    T y_;
};

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d<T> operator*(T scalar, CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T dot(CASTLE_CONST vector2d<T>& first, CASTLE_CONST vector2d<T>& second) CASTLE_NOEXCEPT
{
    return first.dot(second);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T cross(CASTLE_CONST vector2d<T>& first, CASTLE_CONST vector2d<T>& second) CASTLE_NOEXCEPT
{
    return first.cross(second);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector2d<T> operator-(CASTLE_CONST point2d<T>& a, CASTLE_CONST point2d<T>& b) CASTLE_NOEXCEPT
{
    return vector2d<T>(static_cast<T>(a.x() - b.x()), static_cast<T>(a.y() - b.y()));
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point2d<T> operator+(CASTLE_CONST point2d<T>& point, CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    return point2d<T>(
        static_cast<T>(point.x() + value.x()),
        static_cast<T>(point.y() + value.y()));
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR point2d<T> operator-(CASTLE_CONST point2d<T>& point, CASTLE_CONST vector2d<T>& value) CASTLE_NOEXCEPT
{
    return point2d<T>(
        static_cast<T>(point.x() - value.x()),
        static_cast<T>(point.y() - value.y()));
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_VECTOR2D_H
