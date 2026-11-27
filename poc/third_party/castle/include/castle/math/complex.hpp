// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file complex.hpp
 * @brief Allocation-free complex number type and elementary complex functions.
 * @details `castle::math::complex<T>` stores a Cartesian real/imaginary pair
 * of a floating-point type and offers the usual arithmetic operators with
 * complex and real operands, magnitude, phase, polar construction, conjugate,
 * exponential, logarithm, principal square root and integer powers. The class
 * is a literal type, so most operations can be evaluated in constant
 * expressions when the underlying scalar helpers allow it. Division uses
 * Smith's algorithm to avoid spurious overflow, and `abs()` is built on
 * `castle::math::hypot()`. Transcendental functions rely on the platform C
 * math library through `exponential.hpp` and `linalg/trigonometry.hpp`.
 *
 * @code
 * #include "castle/math/complex.hpp"
 *
 * const castle::math::complex<float> z(3.0F, 4.0F);
 * const float magnitude = castle::math::abs(z);                 // 5
 * const float phase = castle::math::arg(z);                     // atan2(4, 3)
 * const auto rotated = z * castle::math::polar(1.0F, 0.5F);     // rotate by 0.5 rad
 * @endcode
 *
 * @note Floating-point types only. The type has no virtual functions, no heap
 *       usage and no exceptions; precondition violations (division by zero,
 *       logarithm of zero) go through `CASTLE_ASSERT`.
 */
#ifndef CASTLE_MATH_COMPLEX_HPP
#define CASTLE_MATH_COMPLEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/exponential.hpp"
#include "castle/math/hypot.hpp"
#include "castle/math/linalg/trigonometry.hpp"
#include "castle/math/powi.hpp"
#include "castle/math/sqrt_real.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Complex number with floating-point Cartesian components.
 * @tparam T Floating-point component type.
 */
template <typename T>
class complex
{
    static_assert(meta::is_floating_point<T>::value,
                  "math::complex requires a floating-point type");

public:
    /** @brief Component type. */
    using value_type = T;

    /**
     * @brief Constructs a complex number from its Cartesian components.
     * @param real Real part (defaults to zero).
     * @param imag Imaginary part (defaults to zero).
     */
    CASTLE_CONSTEXPR complex(T real = static_cast<T>(0), T imag = static_cast<T>(0)) CASTLE_NOEXCEPT
        : real_(real)
        , imag_(imag)
    {
    }

    /**
     * @brief Converts from a complex number with a different component type.
     * @tparam U Source component type.
     * @param other Source value.
     */
    template <typename U>
    CASTLE_CONSTEXPR explicit complex(CASTLE_CONST complex<U>& other) CASTLE_NOEXCEPT
        : real_(static_cast<T>(other.real()))
        , imag_(static_cast<T>(other.imag()))
    {
    }

    /** @brief Returns the real part. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T real() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return real_;
    }

    /** @brief Returns the imaginary part. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T imag() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return imag_;
    }

    /** @brief Replaces the real part. */
    CASTLE_CONSTEXPR void real(T value) CASTLE_NOEXCEPT
    {
        real_ = value;
    }

    /** @brief Replaces the imaginary part. */
    CASTLE_CONSTEXPR void imag(T value) CASTLE_NOEXCEPT
    {
        imag_ = value;
    }

    /** @brief Returns the value unchanged. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR complex operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    /** @brief Returns the negated value. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR complex operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return complex(-real_, -imag_);
    }

    /** @brief Assigns a real value, clearing the imaginary part. */
    CASTLE_CONSTEXPR complex& operator=(T value) CASTLE_NOEXCEPT
    {
        real_ = value;
        imag_ = static_cast<T>(0);
        return *this;
    }

    /** @brief Adds a real value. */
    CASTLE_CONSTEXPR complex& operator+=(T value) CASTLE_NOEXCEPT
    {
        real_ += value;
        return *this;
    }

    /** @brief Subtracts a real value. */
    CASTLE_CONSTEXPR complex& operator-=(T value) CASTLE_NOEXCEPT
    {
        real_ -= value;
        return *this;
    }

    /** @brief Multiplies by a real value. */
    CASTLE_CONSTEXPR complex& operator*=(T value) CASTLE_NOEXCEPT
    {
        real_ *= value;
        imag_ *= value;
        return *this;
    }

    /**
     * @brief Divides by a real value.
     * @warning `value` must not be zero.
     */
    CASTLE_CONSTEXPR complex& operator/=(T value) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(value != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::complex: division by zero"));
        real_ /= value;
        imag_ /= value;
        return *this;
    }

    /** @brief Adds a complex value. */
    CASTLE_CONSTEXPR complex& operator+=(CASTLE_CONST complex& other) CASTLE_NOEXCEPT
    {
        real_ += other.real_;
        imag_ += other.imag_;
        return *this;
    }

    /** @brief Subtracts a complex value. */
    CASTLE_CONSTEXPR complex& operator-=(CASTLE_CONST complex& other) CASTLE_NOEXCEPT
    {
        real_ -= other.real_;
        imag_ -= other.imag_;
        return *this;
    }

    /** @brief Multiplies by a complex value. */
    CASTLE_CONSTEXPR complex& operator*=(CASTLE_CONST complex& other) CASTLE_NOEXCEPT
    {
        CASTLE_CONST T re = real_ * other.real_ - imag_ * other.imag_;
        CASTLE_CONST T im = real_ * other.imag_ + imag_ * other.real_;
        real_ = re;
        imag_ = im;
        return *this;
    }

    /**
     * @brief Divides by a complex value using Smith's algorithm.
     * @warning `other` must not be zero.
     */
    CASTLE_CONSTEXPR complex& operator/=(CASTLE_CONST complex& other) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(other.real_ != static_cast<T>(0) || other.imag_ != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::complex: division by zero"));
        CASTLE_CONST T c = other.real_;
        CASTLE_CONST T d = other.imag_;
        T re = static_cast<T>(0);
        T im = static_cast<T>(0);
        if (castle::math::abs(c) >= castle::math::abs(d))
        {
            CASTLE_CONST T ratio = d / c;
            CASTLE_CONST T denominator = c + d * ratio;
            re = (real_ + imag_ * ratio) / denominator;
            im = (imag_ - real_ * ratio) / denominator;
        }
        else
        {
            CASTLE_CONST T ratio = c / d;
            CASTLE_CONST T denominator = c * ratio + d;
            re = (real_ * ratio + imag_) / denominator;
            im = (imag_ * ratio - real_) / denominator;
        }
        real_ = re;
        imag_ = im;
        return *this;
    }

private:
    T real_;
    T imag_;
};

/** @brief Single-precision complex number. */
using complexf = complex<float>;
/** @brief Double-precision complex number. */
using complexd = complex<double>;

/** @brief Returns `lhs + rhs`. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator+(complex<T> lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    lhs += rhs;
    return lhs;
}

/** @brief Returns `lhs + rhs` with a real right operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator+(complex<T> lhs, meta::type_identity_t<T> rhs) CASTLE_NOEXCEPT
{
    lhs += rhs;
    return lhs;
}

/** @brief Returns `lhs + rhs` with a real left operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator+(meta::type_identity_t<T> lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    return complex<T>(lhs + rhs.real(), rhs.imag());
}

/** @brief Returns `lhs - rhs`. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator-(complex<T> lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    lhs -= rhs;
    return lhs;
}

/** @brief Returns `lhs - rhs` with a real right operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator-(complex<T> lhs, meta::type_identity_t<T> rhs) CASTLE_NOEXCEPT
{
    lhs -= rhs;
    return lhs;
}

/** @brief Returns `lhs - rhs` with a real left operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator-(meta::type_identity_t<T> lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    return complex<T>(lhs - rhs.real(), -rhs.imag());
}

/** @brief Returns `lhs * rhs`. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator*(complex<T> lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    lhs *= rhs;
    return lhs;
}

/** @brief Returns `lhs * rhs` with a real right operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator*(complex<T> lhs, meta::type_identity_t<T> rhs) CASTLE_NOEXCEPT
{
    lhs *= rhs;
    return lhs;
}

/** @brief Returns `lhs * rhs` with a real left operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator*(meta::type_identity_t<T> lhs, complex<T> rhs) CASTLE_NOEXCEPT
{
    rhs *= lhs;
    return rhs;
}

/**
 * @brief Returns `lhs / rhs`.
 * @warning `rhs` must not be zero.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator/(complex<T> lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    lhs /= rhs;
    return lhs;
}

/**
 * @brief Returns `lhs / rhs` with a real right operand.
 * @warning `rhs` must not be zero.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator/(complex<T> lhs, meta::type_identity_t<T> rhs) CASTLE_NOEXCEPT
{
    lhs /= rhs;
    return lhs;
}

/**
 * @brief Returns `lhs / rhs` with a real left operand.
 * @warning `rhs` must not be zero.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> operator/(meta::type_identity_t<T> lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    return complex<T>(lhs) / rhs;
}

/** @brief Exact component-wise equality. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
bool operator==(CASTLE_CONST complex<T>& lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    return (lhs.real() == rhs.real()) && (lhs.imag() == rhs.imag());
}

/** @brief Exact component-wise equality with a real right operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
bool operator==(CASTLE_CONST complex<T>& lhs, meta::type_identity_t<T> rhs) CASTLE_NOEXCEPT
{
    return (lhs.real() == rhs) && (lhs.imag() == static_cast<T>(0));
}

/** @brief Exact component-wise inequality. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
bool operator!=(CASTLE_CONST complex<T>& lhs, CASTLE_CONST complex<T>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/** @brief Exact component-wise inequality with a real right operand. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
bool operator!=(CASTLE_CONST complex<T>& lhs, meta::type_identity_t<T> rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/** @brief Returns the real part of `value`. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T real(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    return value.real();
}

/** @brief Returns the imaginary part of `value`. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T imag(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    return value.imag();
}

/** @brief Returns the complex conjugate of `value`. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
complex<T> conj(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    return complex<T>(value.real(), -value.imag());
}

/**
 * @brief Returns the squared magnitude `re^2 + im^2`.
 * @note Cheaper than `abs()` and free of square roots; may overflow for
 *       components above `sqrt(max())`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T norm(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    return value.real() * value.real() + value.imag() * value.imag();
}

/**
 * @brief Returns the magnitude (modulus) of `value`.
 * @note Computed with `hypot()`, so intermediate overflow is avoided.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
abs(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    return castle::math::hypot(value.real(), value.imag());
}

/**
 * @brief Returns the phase angle of `value` in radians.
 * @return `atan2(imag, real)` in `[-pi, pi]`.
 */
template <typename T>
CASTLE_NODISCARD
T arg(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    return castle::math::radians_atan2(value.imag(), value.real());
}

/**
 * @brief Builds a complex number from polar coordinates.
 * @param magnitude Modulus.
 * @param phase Angle in radians.
 * @return `magnitude * (cos(phase) + i sin(phase))`.
 */
template <typename T>
CASTLE_NODISCARD
complex<T> polar(T magnitude, T phase = static_cast<T>(0)) CASTLE_NOEXCEPT
{
    CASTLE_CONST sin_cos_result<T> sc = castle::math::sin_cos(phase);
    return complex<T>(magnitude * sc.cosine, magnitude * sc.sine);
}

/**
 * @brief Returns `e^(i * phase)`, the unit-magnitude complex number at angle `phase`.
 * @note Equal to `polar(1, phase)`.
 */
template <typename T>
CASTLE_NODISCARD
complex<T> cis(T phase) CASTLE_NOEXCEPT
{
    CASTLE_CONST sin_cos_result<T> sc = castle::math::sin_cos(phase);
    return complex<T>(sc.cosine, sc.sine);
}

/** @brief Returns `e` raised to the complex power `value`. */
template <typename T>
CASTLE_NODISCARD
complex<T> exp(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    return castle::math::polar(castle::math::exp(value.real()), value.imag());
}

/**
 * @brief Returns the principal natural logarithm of `value`.
 * @return `log|value| + i arg(value)`, with imaginary part in `[-pi, pi]`.
 * @warning `value` must not be zero.
 */
template <typename T>
CASTLE_NODISCARD
complex<T> log(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(value.real() != static_cast<T>(0) || value.imag() != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::log: logarithm of zero"));
    return complex<T>(castle::math::log(castle::math::abs(value)), castle::math::arg(value));
}

/**
 * @brief Returns the principal square root of `value`.
 * @return The root with a non-negative real part.
 * @note Uses a cancellation-free formulation for both half-planes.
 */
template <typename T>
CASTLE_NODISCARD
complex<T> sqrt(CASTLE_CONST complex<T>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST T magnitude = castle::math::abs(value);
    if (magnitude == static_cast<T>(0))
    {
        return complex<T>();
    }

    CASTLE_CONST T two = static_cast<T>(2);
    if (value.real() >= static_cast<T>(0))
    {
        CASTLE_CONST T root = castle::math::sqrt_real((magnitude + value.real()) / two);
        return complex<T>(root, value.imag() / (two * root));
    }

    CASTLE_CONST T root = castle::math::sqrt_real((magnitude - value.real()) / two);
    return complex<T>(castle::math::abs(value.imag()) / (two * root),
                      (value.imag() < static_cast<T>(0)) ? -root : root);
}

/**
 * @brief Raises `base` to an integer power by repeated squaring.
 * @param base Complex base.
 * @param exponent Signed integer exponent.
 * @return `base^exponent`; negative exponents return the reciprocal.
 * @warning A zero base with a negative exponent divides by zero.
 */
template <typename T>
CASTLE_NODISCARD
complex<T> pow(CASTLE_CONST complex<T>& base, int exponent) CASTLE_NOEXCEPT
{
    if (exponent >= 0)
    {
        return castle::math::powi(base, static_cast<unsigned>(exponent));
    }

    // Negate in unsigned arithmetic so INT_MIN stays well defined.
    CASTLE_CONST unsigned magnitude = 0U - static_cast<unsigned>(exponent);
    return complex<T>(static_cast<T>(1)) / castle::math::powi(base, magnitude);
}

/**
 * @brief Compares two complex numbers with a caller-supplied tolerance.
 * @param a First value.
 * @param b Second value.
 * @param epsilon Non-negative tolerance.
 * @return `true` when `abs(a - b) <= epsilon * max(1, abs(a), abs(b))`.
 * @note Complex counterpart of the scalar `near_equal()` in `near_equal.hpp`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
bool near_equal(CASTLE_CONST complex<T>& a, CASTLE_CONST complex<T>& b, T epsilon) CASTLE_NOEXCEPT
{
    CASTLE_CONST T difference = castle::math::abs(a - b);
    CASTLE_CONST T scale = castle::max(static_cast<T>(1),
                                       castle::max(castle::math::abs(a), castle::math::abs(b)));
    return difference <= epsilon * scale;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_COMPLEX_HPP
