#ifndef CASTLE_MATH_VECTOR_H
#define CASTLE_MATH_VECTOR_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/math/abs.h"
#include "castle/math/lerp.h"
#include "castle/math/near_equal.h"
#include "castle/math/sqrt_real.h"

namespace castle
{
namespace math
{

// ============================================================================
// Fixed-size mathematical vector.
//
// Storage is embedded directly in the object. No allocation, exceptions or
// RTTI are used. The implementation is intentionally independent from the
// STL and uses castle's arithmetic traits and error handling conventions.
//
// The vector is useful for control, robotics, graphics, geometry and sensor
// processing where the dimension is known at compile time.
// ============================================================================
template <typename T, size_type N>
class vector
{
    static_assert(N > 0U, "math::vector requires a positive dimension");
    static_assert(meta::is_arithmetic<T>::value,
                  "math::vector requires an arithmetic type");

public:
    using value_type = T;
    using size_type = castle::size_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using iterator = T*;
    using const_iterator = CASTLE_CONST T*;

    static CASTLE_CONSTEXPR size_type static_size = N;

    CASTLE_CONSTEXPR vector() CASTLE_NOEXCEPT : data_{}
    {
    }

    template <typename... Args,
              meta::enable_if_t<(sizeof...(Args) == N) &&
                                meta::conjunction<meta::is_constructible<T, Args&&>...>::value, int> = 0>
    CASTLE_CONSTEXPR explicit vector(Args&&... args)
        : data_{static_cast<T>(args)...}
    {
    }

    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return data_ + N; }
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }

    CASTLE_CONSTEXPR reference operator[](size_type index) CASTLE_NOEXCEPT
    {
        return data_[index];
    }

    CASTLE_CONSTEXPR const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_[index];
    }

    CASTLE_CONSTEXPR reference at(size_type index) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < N,
                      CASTLE_ERROR_GENERIC("castle::math::vector::at: index out of range"));
        return data_[index];
    }

    CASTLE_CONSTEXPR const_reference at(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < N,
                      CASTLE_ERROR_GENERIC("castle::math::vector::at: index out of range"));
        return data_[index];
    }

    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    CASTLE_CONSTEXPR vector operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    CASTLE_CONSTEXPR vector operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(-data_[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR vector operator+(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] + other[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR vector operator-(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] - other[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR vector& operator+=(CASTLE_CONST vector& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] + other[i]);
        }
        return *this;
    }

    CASTLE_CONSTEXPR vector& operator-=(CASTLE_CONST vector& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] - other[i]);
        }
        return *this;
    }

    CASTLE_CONSTEXPR vector operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] * scalar);
        }
        return result;
    }

    CASTLE_NODISCARD vector operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0),
                      CASTLE_ERROR_GENERIC("castle::math::vector: division by zero"));

        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] / scalar);
        }
        return result;
    }

    CASTLE_CONSTEXPR vector& operator*=(T scalar) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] * scalar);
        }
        return *this;
    }

    vector& operator/=(T scalar) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0),
                      CASTLE_ERROR_GENERIC("castle::math::vector: division by zero"));

        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] / scalar);
        }
        return *this;
    }

    CASTLE_CONSTEXPR bool operator==(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            if (data_[i] != other[i])
            {
                return false;
            }
        }
        return true;
    }

    CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

    CASTLE_CONSTEXPR T squared_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        T result{};
        for (size_type i = 0U; i < N; ++i)
        {
            result = static_cast<T>(result + data_[i] * data_[i]);
        }
        return result;
    }

    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return sqrt_real(static_cast<U>(squared_length()));
    }

    CASTLE_CONSTEXPR T dot(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        T result{};
        for (size_type i = 0U; i < N; ++i)
        {
            result = static_cast<T>(result + data_[i] * other[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR vector hadamard(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] * other[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR vector component_min(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = (data_[i] < other[i]) ? data_[i] : other[i];
        }
        return result;
    }

    CASTLE_CONSTEXPR vector component_max(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = (data_[i] > other[i]) ? data_[i] : other[i];
        }
        return result;
    }

    CASTLE_NODISCARD vector abs() CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = castle::math::abs(data_[i]);
        }
        return result;
    }

    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, vector>::type
    normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST U len = length<U>();
        CASTLE_ASSERT(len > static_cast<U>(0),
                      CASTLE_ERROR_GENERIC("castle::math::vector::normalized: zero vector"));
        return *this / static_cast<T>(len);
    }

    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    distance_to(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (*this - other).template length<U>();
    }

    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, vector>::type
    lerp_to(CASTLE_CONST vector& other, U t) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(castle::math::lerp(
                static_cast<U>(data_[i]),
                static_cast<U>(other[i]),
                t));
        }
        return result;
    }

private:
    T data_[N];
};

template <typename T, size_type N>
CASTLE_CONSTEXPR vector<T, N> operator*(T scalar, CASTLE_CONST vector<T, N>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

template <typename T>
using vector2 = vector<T, 2U>;

template <typename T>
using vector3 = vector<T, 3U>;

template <typename T>
using vector4 = vector<T, 4U>;

template <typename T, size_type N>
CASTLE_CONSTEXPR T dot(CASTLE_CONST vector<T, N>& first, CASTLE_CONST vector<T, N>& second) CASTLE_NOEXCEPT
{
    return first.dot(second);
}

template <typename T, size_type N>
CASTLE_CONSTEXPR vector<T, N> hadamard(CASTLE_CONST vector<T, N>& first, CASTLE_CONST vector<T, N>& second) CASTLE_NOEXCEPT
{
    return first.hadamard(second);
}

template <typename T, size_type N>
CASTLE_NODISCARD typename meta::enable_if<(meta::is_arithmetic<T>::value), vector<T, N>>::type
project(CASTLE_CONST vector<T, N>& value, CASTLE_CONST vector<T, N>& onto) CASTLE_NOEXCEPT
{
    CASTLE_CONST T denominator = onto.squared_length();
    CASTLE_ASSERT(denominator != static_cast<T>(0),
                  CASTLE_ERROR_GENERIC("castle::math::project: zero basis vector"));
    return onto * static_cast<T>(value.dot(onto) / denominator);
}

template <typename T, size_type N>
CASTLE_CONSTEXPR vector<T, N> reflect(
    CASTLE_CONST vector<T, N>& value,
    CASTLE_CONST vector<T, N>& normal) CASTLE_NOEXCEPT
{
    return value - normal * static_cast<T>(2 * value.dot(normal));
}

template <typename T>
CASTLE_CONSTEXPR vector<T, 3U> cross(
    CASTLE_CONST vector<T, 3U>& first,
    CASTLE_CONST vector<T, 3U>& second) CASTLE_NOEXCEPT
{
    return vector<T, 3U>(
        static_cast<T>(first[1U] * second[2U] - first[2U] * second[1U]),
        static_cast<T>(first[2U] * second[0U] - first[0U] * second[2U]),
        static_cast<T>(first[0U] * second[1U] - first[1U] * second[0U]));
}

template <typename T>
CASTLE_CONSTEXPR T scalar_triple_product(
    CASTLE_CONST vector<T, 3U>& first,
    CASTLE_CONST vector<T, 3U>& second,
    CASTLE_CONST vector<T, 3U>& third) CASTLE_NOEXCEPT
{
    return dot(first, cross(second, third));
}

template <typename T, size_type N>
CASTLE_NODISCARD bool near_equal(
    CASTLE_CONST vector<T, N>& first,
    CASTLE_CONST vector<T, N>& second,
    T epsilon) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "vector near_equal requires floating-point elements");

    for (size_type i = 0U; i < N; ++i)
    {
        if (!castle::math::near_equal(first[i], second[i], epsilon))
        {
            return false;
        }
    }
    return true;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_VECTOR_H
