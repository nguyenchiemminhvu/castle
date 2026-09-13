#ifndef CASTLE_MATH_QUATERNION_H
#define CASTLE_MATH_QUATERNION_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/angle.h"
#include "castle/math/sqrt_real.h"
#include "castle/math/linalg/matrix.h"
#include "castle/math/linalg/trigonometry.h"
#include "castle/math/linalg/vector.h"

namespace castle
{
namespace math
{

// ============================================================================
// Unit/quaternion rotation primitive.
//
// Layout is (w, x, y, z). Multiplication composes rotations in the usual
// Hamilton convention. Quaternion storage is fixed and allocation-free.
// ============================================================================
template <typename T>
class quaternion
{
    static_assert(meta::is_floating_point<T>::value,
                  "math::quaternion requires a floating-point type");

public:
    using value_type = T;

    CASTLE_CONSTEXPR quaternion() CASTLE_NOEXCEPT
        : w_(static_cast<T>(1)), x_(static_cast<T>(0)), y_(static_cast<T>(0)), z_(static_cast<T>(0))
    {
    }

    CASTLE_CONSTEXPR quaternion(T w, T x, T y, T z) CASTLE_NOEXCEPT
        : w_(w), x_(x), y_(y), z_(z)
    {
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T w() CASTLE_CONST CASTLE_NOEXCEPT { return w_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T x() CASTLE_CONST CASTLE_NOEXCEPT { return x_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T y() CASTLE_CONST CASTLE_NOEXCEPT { return y_; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR T z() CASTLE_CONST CASTLE_NOEXCEPT { return z_; }

    CASTLE_NODISCARD CASTLE_CONSTEXPR T squared_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(w_ * w_ + x_ * x_ + y_ * y_ + z_ * z_);
    }

    CASTLE_NODISCARD T length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return sqrt_real(squared_length());
    }

    CASTLE_NODISCARD quaternion conjugate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return quaternion(w_, static_cast<T>(-x_), static_cast<T>(-y_), static_cast<T>(-z_));
    }

    CASTLE_NODISCARD quaternion normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST T len = length();
        CASTLE_ASSERT(len > static_cast<T>(0),
                      CASTLE_ERROR_GENERIC("castle::math::quaternion::normalized: zero quaternion"));
        return quaternion(
            static_cast<T>(w_ / len),
            static_cast<T>(x_ / len),
            static_cast<T>(y_ / len),
            static_cast<T>(z_ / len));
    }

    CASTLE_NODISCARD quaternion inverse() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST T norm = squared_length();
        CASTLE_ASSERT(norm > static_cast<T>(0),
                      CASTLE_ERROR_GENERIC("castle::math::quaternion::inverse: zero quaternion"));
        return conjugate() / norm;
    }

    CASTLE_NODISCARD quaternion operator*(CASTLE_CONST quaternion& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return quaternion(
            static_cast<T>(w_ * other.w_ - x_ * other.x_ - y_ * other.y_ - z_ * other.z_),
            static_cast<T>(w_ * other.x_ + x_ * other.w_ + y_ * other.z_ - z_ * other.y_),
            static_cast<T>(w_ * other.y_ - x_ * other.z_ + y_ * other.w_ + z_ * other.x_),
            static_cast<T>(w_ * other.z_ + x_ * other.y_ - y_ * other.x_ + z_ * other.w_));
    }

    CASTLE_NODISCARD quaternion operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0),
                      CASTLE_ERROR_GENERIC("castle::math::quaternion: division by zero"));
        return quaternion(
            static_cast<T>(w_ / scalar),
            static_cast<T>(x_ / scalar),
            static_cast<T>(y_ / scalar),
            static_cast<T>(z_ / scalar));
    }

    CASTLE_NODISCARD vector<T, 3U> rotate(CASTLE_CONST vector<T, 3U>& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST quaternion q = normalized();
        CASTLE_CONST quaternion p(static_cast<T>(0), value[0U], value[1U], value[2U]);
        CASTLE_CONST quaternion rotated = q * p * q.conjugate();
        return vector<T, 3U>(rotated.x_, rotated.y_, rotated.z_);
    }

    CASTLE_NODISCARD matrix<T, 3U, 3U> to_matrix() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST quaternion q = normalized();
        CASTLE_CONST T xx = static_cast<T>(q.x_ * q.x_);
        CASTLE_CONST T yy = static_cast<T>(q.y_ * q.y_);
        CASTLE_CONST T zz = static_cast<T>(q.z_ * q.z_);
        CASTLE_CONST T xy = static_cast<T>(q.x_ * q.y_);
        CASTLE_CONST T xz = static_cast<T>(q.x_ * q.z_);
        CASTLE_CONST T yz = static_cast<T>(q.y_ * q.z_);
        CASTLE_CONST T wx = static_cast<T>(q.w_ * q.x_);
        CASTLE_CONST T wy = static_cast<T>(q.w_ * q.y_);
        CASTLE_CONST T wz = static_cast<T>(q.w_ * q.z_);
        CASTLE_CONST T two = static_cast<T>(2);

        return matrix<T, 3U, 3U>(
            static_cast<T>(1 - two * (yy + zz)), static_cast<T>(two * (xy - wz)),     static_cast<T>(two * (xz + wy)),
            static_cast<T>(two * (xy + wz)),     static_cast<T>(1 - two * (xx + zz)), static_cast<T>(two * (yz - wx)),
            static_cast<T>(two * (xz - wy)),     static_cast<T>(two * (yz + wx)),     static_cast<T>(1 - two * (xx + yy)));
    }

    CASTLE_CONSTEXPR bool operator==(CASTLE_CONST quaternion& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return w_ == other.w_ && x_ == other.x_ && y_ == other.y_ && z_ == other.z_;
    }

    CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST quaternion& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

    static quaternion from_axis_angle(CASTLE_CONST vector<T, 3U>& axis, T radians) CASTLE_NOEXCEPT
    {
        CASTLE_CONST vector<T, 3U> normalized_axis = axis.normalized();
        CASTLE_CONST T half_angle = static_cast<T>(radians * static_cast<T>(0.5));
        CASTLE_CONST T sine = castle::math::sin(half_angle);
        CASTLE_CONST T cosine = castle::math::cos(half_angle);
        return quaternion(
            cosine,
            static_cast<T>(normalized_axis[0U] * sine),
            static_cast<T>(normalized_axis[1U] * sine),
            static_cast<T>(normalized_axis[2U] * sine));
    }

    static quaternion from_axis_angle_degrees(CASTLE_CONST vector<T, 3U>& axis, T degrees) CASTLE_NOEXCEPT
    {
        return from_axis_angle(axis, degrees_to_radians(degrees));
    }

    static quaternion from_euler_xyz(T x_radians, T y_radians, T z_radians) CASTLE_NOEXCEPT
    {
        CASTLE_CONST T hx = static_cast<T>(x_radians * static_cast<T>(0.5));
        CASTLE_CONST T hy = static_cast<T>(y_radians * static_cast<T>(0.5));
        CASTLE_CONST T hz = static_cast<T>(z_radians * static_cast<T>(0.5));
        CASTLE_CONST T sx = castle::math::sin(hx);
        CASTLE_CONST T cx = castle::math::cos(hx);
        CASTLE_CONST T sy = castle::math::sin(hy);
        CASTLE_CONST T cy = castle::math::cos(hy);
        CASTLE_CONST T sz = castle::math::sin(hz);
        CASTLE_CONST T cz = castle::math::cos(hz);

        return quaternion(
            static_cast<T>(cx * cy * cz + sx * sy * sz),
            static_cast<T>(sx * cy * cz - cx * sy * sz),
            static_cast<T>(cx * sy * cz + sx * cy * sz),
            static_cast<T>(cx * cy * sz - sx * sy * cz));
    }

private:
    T w_;
    T x_;
    T y_;
    T z_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_QUATERNION_H
