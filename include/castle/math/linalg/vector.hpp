// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file
 * @brief Fixed-size vector primitives for Castle linear algebra.
 *
 * This header provides a compile-time-sized vector class and related helpers
 * for embedded numeric code, geometry, and control pipelines. Use it when the
 * dimension is known at compile time and you need deterministic storage,
 * bounds-checked access through Castle assertions, and no dependence on heap
 * allocation, exceptions, RTTI, virtual dispatch, or the STL. Norms and
 * normalization are available only for floating-point element types and use
 * `castle::math::sqrt_real`; projection and interpolation preserve the exact
 * arithmetic semantics of the chosen scalar type.
 *
 * @code
 * #include "castle/math/linalg/vector.hpp"
 *
 * const castle::math::vector<float, 3U> a(1.0F, 2.0F, 3.0F);
 * const castle::math::vector<float, 3U> b(4.0F, 5.0F, 6.0F);
 * const float dot_value = a.dot(b);
 * const castle::math::vector<float, 3U> unit = a.normalized();
 * @endcode
 */
#ifndef CASTLE_MATH_VECTOR_HPP
#define CASTLE_MATH_VECTOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/lerp.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/sqrt_real.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Fixed-size mathematical vector with inline contiguous storage.
 * @tparam T Arithmetic element type stored in each component.
 * @tparam N Compile-time number of components.
 * @note Elements are stored contiguously in index order inside the object.
 * @warning `operator[]` does not perform bounds checking; use `at()` when the
 * caller cannot guarantee a valid index.
 */
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

    /** @brief Compile-time vector dimension exposed as a constant. */
    static CASTLE_CONSTEXPR size_type static_size = N;

    /**
     * @brief Constructs a zero-initialized vector.
     * @return A vector whose components are value-initialized.
     * @note All storage lives inside the object with no allocation.
     */
    CASTLE_CONSTEXPR vector() CASTLE_NOEXCEPT : data_{}
    {
    }

    /**
     * @brief Constructs a vector from exactly `N` component values.
     * @tparam Args Component argument types convertible to `T`.
     * @param args Component values in ascending index order.
     * @return A vector initialized from the supplied components.
     * @warning The constructor participates in overload resolution only when
     * exactly `N` arguments are provided.
     */
    template <typename... Args,
              meta::enable_if_t<(sizeof...(Args) == N) &&
                                meta::conjunction<meta::is_constructible<T, Args&&>...>::value, int> = 0>
    CASTLE_CONSTEXPR explicit vector(Args&&... args)
        : data_{static_cast<T>(args)...}
    {
    }

    /**
     * @brief Returns the number of stored components.
     * @return The compile-time dimension `N`.
     * @note This operation is constant-time and `constexpr`.
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns an iterator to the first component.
     * @return Pointer to the first stored component.
     * @note Iteration follows contiguous index order.
     */
    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first component.
     * @return Const pointer to the first stored component.
     * @note Iteration follows contiguous index order.
     */
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first component.
     * @return Const pointer to the first stored component.
     * @note Equivalent to `begin()` on a const vector.
     */
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an iterator one past the last component.
     * @return Pointer one past the stored component range.
     * @note Iteration follows contiguous index order.
     */
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return data_ + N; }

    /**
     * @brief Returns a const iterator one past the last component.
     * @return Const pointer one past the stored component range.
     * @note Iteration follows contiguous index order.
     */
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }

    /**
     * @brief Returns a const iterator one past the last component.
     * @return Const pointer one past the stored component range.
     * @note Equivalent to `end()` on a const vector.
     */
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }

    /**
     * @brief Returns the component at `index` without bounds checking.
     * @param index Zero-based component index.
     * @return Mutable reference to the selected component.
     * @warning Passing an out-of-range index is undefined for this API.
     */
    CASTLE_CONSTEXPR reference operator[](size_type index) CASTLE_NOEXCEPT
    {
        return data_[index];
    }

    /**
     * @brief Returns the component at `index` without bounds checking.
     * @param index Zero-based component index.
     * @return Const reference to the selected component.
     * @warning Passing an out-of-range index is undefined for this API.
     */
    CASTLE_CONSTEXPR const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_[index];
    }

    /**
     * @brief Returns the component at `index` with Castle assertion checking.
     * @param index Zero-based component index.
     * @return Mutable reference to the selected component.
     * @warning Triggers Castle's assertion/error path when `index >= N`.
     */
    CASTLE_CONSTEXPR reference at(size_type index) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < N, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::vector::at: index out of range"));
        return data_[index];
    }

    /**
     * @brief Returns the component at `index` with Castle assertion checking.
     * @param index Zero-based component index.
     * @return Const reference to the selected component.
     * @warning Triggers Castle's assertion/error path when `index >= N`.
     */
    CASTLE_CONSTEXPR const_reference at(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < N, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::vector::at: index out of range"));
        return data_[index];
    }

    /**
     * @brief Returns the contiguous storage pointer.
     * @return Pointer to the first component.
     * @note The returned pointer remains valid for the lifetime of the vector.
     */
    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns the contiguous storage pointer.
     * @return Const pointer to the first component.
     * @note The returned pointer remains valid for the lifetime of the vector.
     */
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an unchanged copy of the vector.
     * @return Copy of `*this`.
     * @note Provided for arithmetic symmetry with unary minus.
     */
    CASTLE_CONSTEXPR vector operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    /**
     * @brief Returns the component-wise negation of the vector.
     * @return Vector whose components are `-data_[i]`.
     * @note The operation is performed independently for each component.
     */
    CASTLE_CONSTEXPR vector operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(-data_[i]);
        }
        return result;
    }

    /**
     * @brief Adds two vectors component-wise.
     * @param other Vector added to `*this`.
     * @return Component-wise sum.
     * @note Both vectors must have the same dimension and element type.
     */
    CASTLE_CONSTEXPR vector operator+(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] + other[i]);
        }
        return result;
    }

    /**
     * @brief Subtracts two vectors component-wise.
     * @param other Vector subtracted from `*this`.
     * @return Component-wise difference.
     * @note Both vectors must have the same dimension and element type.
     */
    CASTLE_CONSTEXPR vector operator-(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] - other[i]);
        }
        return result;
    }

    /**
     * @brief Adds another vector into this vector component-wise.
     * @param other Vector added to `*this`.
     * @return Reference to `*this`.
     * @note The update is performed in place with no temporary allocation.
     */
    CASTLE_CONSTEXPR vector& operator+=(CASTLE_CONST vector& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] + other[i]);
        }
        return *this;
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param other Vector subtracted from `*this`.
     * @return Reference to `*this`.
     * @note The update is performed in place with no temporary allocation.
     */
    CASTLE_CONSTEXPR vector& operator-=(CASTLE_CONST vector& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] - other[i]);
        }
        return *this;
    }

    /**
     * @brief Multiplies every component by `scalar`.
     * @param scalar Scalar multiplier.
     * @return Scaled vector.
     * @note Arithmetic follows the rules of `T`.
     */
    CASTLE_CONSTEXPR vector operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] * scalar);
        }
        return result;
    }

    /**
     * @brief Divides every component by `scalar`.
     * @param scalar Scalar divisor.
     * @return Scaled vector.
     * @warning Triggers Castle's assertion/error path when `scalar == 0`.
     */
    CASTLE_NODISCARD vector operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::vector: division by zero"));

        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] / scalar);
        }
        return result;
    }

    /**
     * @brief Multiplies every component by `scalar` in place.
     * @param scalar Scalar multiplier.
     * @return Reference to `*this`.
     * @note Arithmetic follows the rules of `T`.
     */
    CASTLE_CONSTEXPR vector& operator*=(T scalar) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] * scalar);
        }
        return *this;
    }

    /**
     * @brief Divides every component by `scalar` in place.
     * @param scalar Scalar divisor.
     * @return Reference to `*this`.
     * @warning Triggers Castle's assertion/error path when `scalar == 0`.
     */
    vector& operator/=(T scalar) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::vector: division by zero"));

        for (size_type i = 0U; i < N; ++i)
        {
            data_[i] = static_cast<T>(data_[i] / scalar);
        }
        return *this;
    }

    /**
     * @brief Compares vectors component by component for exact equality.
     * @param other Vector compared with `*this`.
     * @return `true` when every component is exactly equal.
     * @warning For floating-point vectors, use `near_equal()` when a tolerance
     * is required.
     */
    CASTLE_CONSTEXPR bool operator==(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            // LCOV_EXCL_START
            if (data_[i] != other[i])
            {
                return false;
            }
            // LCOV_EXCL_STOP
        }
        return true;
    }

    /**
     * @brief Compares vectors component by component for exact inequality.
     * @param other Vector compared with `*this`.
     * @return `true` when any component differs exactly.
     * @warning For floating-point vectors, use `near_equal()` when a tolerance
     * is required.
     */
    CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

    /**
     * @brief Returns the sum of squared component magnitudes.
     * @return `sum(data_[i] * data_[i])`.
     * @note This avoids a square root and is available for any arithmetic `T`.
     */
    CASTLE_CONSTEXPR T squared_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        T result{};
        for (size_type i = 0U; i < N; ++i)
        {
            result = static_cast<T>(result + data_[i] * data_[i]);
        }
        return result;
    }

    /**
     * @brief Returns the Euclidean length of the vector.
     * @tparam U Floating-point computation type, defaulting to `T`.
     * @return `sqrt_real(squared_length())`.
     * @warning Available only for floating-point element types.
     */
    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return sqrt_real(static_cast<U>(squared_length()));
    }

    /**
     * @brief Returns the dot product with another vector.
     * @param other Right-hand vector operand.
     * @return Sum of pairwise products.
     * @note The result type is `T`.
     */
    CASTLE_CONSTEXPR T dot(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        T result{};
        for (size_type i = 0U; i < N; ++i)
        {
            result = static_cast<T>(result + data_[i] * other[i]);
        }
        return result;
    }

    /**
     * @brief Returns the component-wise product with another vector.
     * @param other Right-hand vector operand.
     * @return Vector whose components are `data_[i] * other[i]`.
     * @note This is also known as the Hadamard product.
     */
    CASTLE_CONSTEXPR vector hadamard(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = static_cast<T>(data_[i] * other[i]);
        }
        return result;
    }

    /**
     * @brief Returns the component-wise minimum with another vector.
     * @param other Right-hand vector operand.
     * @return Vector containing `min(data_[i], other[i])`.
     * @note Comparison uses the built-in `<` operator on `T`.
     */
    CASTLE_CONSTEXPR vector component_min(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = (data_[i] < other[i]) ? data_[i] : other[i];
        }
        return result;
    }

    /**
     * @brief Returns the component-wise maximum with another vector.
     * @param other Right-hand vector operand.
     * @return Vector containing `max(data_[i], other[i])`.
     * @note Comparison uses the built-in `>` operator on `T`.
     */
    CASTLE_CONSTEXPR vector component_max(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = (data_[i] > other[i]) ? data_[i] : other[i];
        }
        return result;
    }

    /**
     * @brief Returns the component-wise absolute value vector.
     * @return Vector containing `castle::math::abs(data_[i])`.
     * @warning Signed integral components still follow `castle::math::abs()`
     * semantics for the minimum representable value.
     */
    CASTLE_NODISCARD vector abs() CASTLE_CONST CASTLE_NOEXCEPT
    {
        vector result;
        for (size_type i = 0U; i < N; ++i)
        {
            result[i] = castle::math::abs(data_[i]);
        }
        return result;
    }

    /**
     * @brief Returns a unit-length copy of the vector.
     * @tparam U Floating-point computation type, defaulting to `T`.
     * @return Vector divided by its Euclidean length.
     * @warning Triggers Castle's assertion/error path for the zero vector and
     * is available only for floating-point element types.
     */
    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, vector>::type
    normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST U len = length<U>();
        CASTLE_ASSERT(len > static_cast<U>(0), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::vector::normalized: zero vector"));
        return *this / static_cast<T>(len);
    }

    /**
     * @brief Returns the Euclidean distance to another vector.
     * @tparam U Floating-point computation type, defaulting to `T`.
     * @param other Vector whose distance from `*this` is measured.
     * @return `(*this - other).length()`.
     * @warning Available only for floating-point element types.
     */
    template <typename U = T>
    CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<U>::value, U>::type
    distance_to(CASTLE_CONST vector& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (*this - other).template length<U>();
    }

    /**
     * @brief Linearly interpolates each component toward another vector.
     * @tparam U Floating-point interpolation parameter type, defaulting to `T`.
     * @param other Target vector.
     * @param t Interpolation factor used as `a + t * (b - a)` for each component.
     * @return Interpolated vector.
     * @warning No clamping is performed on `t`.
     */
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

/**
 * @brief Multiplies a vector by a scalar with the scalar on the left.
 * @tparam T Vector element type.
 * @tparam N Compile-time vector dimension.
 * @param scalar Scalar multiplier.
 * @param value Vector operand.
 * @return `value * scalar`.
 */
template <typename T, size_type N>
CASTLE_CONSTEXPR vector<T, N> operator*(T scalar, CASTLE_CONST vector<T, N>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

/** @brief Convenience alias for a two-component vector. */
template <typename T>
using vector2 = vector<T, 2U>;

/** @brief Convenience alias for a three-component vector. */
template <typename T>
using vector3 = vector<T, 3U>;

/** @brief Convenience alias for a four-component vector. */
template <typename T>
using vector4 = vector<T, 4U>;

/**
 * @brief Returns the dot product of two vectors.
 * @tparam T Vector element type.
 * @tparam N Compile-time vector dimension.
 * @param first Left-hand vector operand.
 * @param second Right-hand vector operand.
 * @return `first.dot(second)`.
 */
template <typename T, size_type N>
CASTLE_CONSTEXPR T dot(CASTLE_CONST vector<T, N>& first, CASTLE_CONST vector<T, N>& second) CASTLE_NOEXCEPT
{
    return first.dot(second);
}

/**
 * @brief Returns the component-wise product of two vectors.
 * @tparam T Vector element type.
 * @tparam N Compile-time vector dimension.
 * @param first Left-hand vector operand.
 * @param second Right-hand vector operand.
 * @return `first.hadamard(second)`.
 */
template <typename T, size_type N>
CASTLE_CONSTEXPR vector<T, N> hadamard(CASTLE_CONST vector<T, N>& first, CASTLE_CONST vector<T, N>& second) CASTLE_NOEXCEPT
{
    return first.hadamard(second);
}

/**
 * @brief Projects one vector onto another.
 * @tparam T Arithmetic vector element type.
 * @tparam N Compile-time vector dimension.
 * @param value Vector being projected.
 * @param onto Basis vector that defines the projection direction.
 * @return Projection of `value` onto `onto`.
 * @warning Triggers Castle's assertion/error path when `onto` is the zero
 * vector. For integral `T`, the division uses integral arithmetic.
 */
template <typename T, size_type N>
CASTLE_NODISCARD typename meta::enable_if<(meta::is_arithmetic<T>::value), vector<T, N>>::type
project(CASTLE_CONST vector<T, N>& value, CASTLE_CONST vector<T, N>& onto) CASTLE_NOEXCEPT
{
    CASTLE_CONST T denominator = onto.squared_length();
    CASTLE_ASSERT(denominator != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::project: zero basis vector"));
    return onto * static_cast<T>(value.dot(onto) / denominator);
}

/**
 * @brief Reflects a vector about a supplied normal.
 * @tparam T Vector element type.
 * @tparam N Compile-time vector dimension.
 * @param value Vector being reflected.
 * @param normal Reflection normal.
 * @return `value - normal * (2 * value.dot(normal))`.
 * @warning The normal is used exactly as provided and is not normalized.
 */
template <typename T, size_type N>
CASTLE_CONSTEXPR vector<T, N> reflect(
    CASTLE_CONST vector<T, N>& value,
    CASTLE_CONST vector<T, N>& normal) CASTLE_NOEXCEPT
{
    return value - normal * static_cast<T>(2 * value.dot(normal));
}

/**
 * @brief Returns the three-dimensional cross product.
 * @tparam T Vector element type.
 * @param first Left-hand vector operand.
 * @param second Right-hand vector operand.
 * @return Vector orthogonal to `first` and `second` using the right-hand rule.
 */
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

/**
 * @brief Returns the scalar triple product of three 3D vectors.
 * @tparam T Vector element type.
 * @param first First vector.
 * @param second Second vector.
 * @param third Third vector.
 * @return `dot(first, cross(second, third))`.
 * @note The result equals the signed volume scale factor of the parallelepiped.
 */
template <typename T>
CASTLE_CONSTEXPR T scalar_triple_product(
    CASTLE_CONST vector<T, 3U>& first,
    CASTLE_CONST vector<T, 3U>& second,
    CASTLE_CONST vector<T, 3U>& third) CASTLE_NOEXCEPT
{
    return dot(first, cross(second, third));
}

/**
 * @brief Compares two floating-point vectors with a relative tolerance.
 * @tparam T Floating-point element type.
 * @tparam N Compile-time vector dimension.
 * @param first Left-hand vector operand.
 * @param second Right-hand vector operand.
 * @param epsilon Relative tolerance forwarded to scalar `near_equal()`.
 * @return `true` when every component compares equal within tolerance.
 * @warning This helper is available only for floating-point element types.
 */
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

#endif
