// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file differentiation.hpp
 * @brief Numerical differentiation of functions and sampled data.
 * @details Finite-difference derivative estimators for any callable `T f(T)`
 * and for uniformly spaced samples:
 *  - `derivative_forward` / `derivative_backward`: first order, one-sided.
 *  - `derivative_central`: second order, symmetric.
 *  - `derivative_five_point`: fourth-order five-point stencil.
 *  - `derivative_richardson`: Richardson extrapolation of central differences.
 *  - `second_derivative`: second-order central estimate of `f''`.
 *  - `derivative_samples`: gradient of sampled data with second-order accuracy.
 *
 * The step `h` is always supplied by the caller. As a rule of thumb the
 * truncation/rounding optimum is about `eps^(1/2) * max(1, |x|)` for one-sided
 * differences and `eps^(1/3) * max(1, |x|)` for central differences.
 *
 * @code
 * #include "castle/math/numeric/differentiation.hpp"
 *
 * auto f = [](double x) { return x * x * x; };
 * const double d1 = castle::math::derivative_central(f, 2.0, 1e-4);   // 12
 * const double d2 = castle::math::second_derivative(f, 2.0, 1e-3);    // 12
 * @endcode
 *
 * @note Callables are invoked as `f(x)` and must be non-throwing. All
 *       routines are allocation-free and non-recursive.
 */
#ifndef CASTLE_MATH_NUMERIC_DIFFERENTIATION_HPP
#define CASTLE_MATH_NUMERIC_DIFFERENTIATION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Forward difference `(f(x + h) - f(x)) / h`.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `x`.
 * @param f Function to differentiate.
 * @param x Evaluation point.
 * @param h Non-zero step.
 * @return First-order accurate estimate of `f'(x)`.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
derivative_forward(F&& f, T x, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "derivative_forward requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(h != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::derivative_forward: step must be non-zero"),
                                  static_cast<T>(0));
    return (f(x + h) - f(x)) / h;
}

/**
 * @brief Backward difference `(f(x) - f(x - h)) / h`.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `x`.
 * @param f Function to differentiate.
 * @param x Evaluation point.
 * @param h Non-zero step.
 * @return First-order accurate estimate of `f'(x)`.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
derivative_backward(F&& f, T x, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "derivative_backward requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(h != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::derivative_backward: step must be non-zero"),
                                  static_cast<T>(0));
    return (f(x) - f(x - h)) / h;
}

/**
 * @brief Central difference `(f(x + h) - f(x - h)) / (2h)`.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `x`.
 * @param f Function to differentiate.
 * @param x Evaluation point.
 * @param h Non-zero step.
 * @return Second-order accurate estimate of `f'(x)`.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
derivative_central(F&& f, T x, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "derivative_central requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(h != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::derivative_central: step must be non-zero"),
                                  static_cast<T>(0));
    return (f(x + h) - f(x - h)) / (static_cast<T>(2) * h);
}

/**
 * @brief Five-point stencil `(f(x-2h) - 8f(x-h) + 8f(x+h) - f(x+2h)) / (12h)`.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `x`.
 * @param f Function to differentiate.
 * @param x Evaluation point.
 * @param h Non-zero step.
 * @return Fourth-order accurate estimate of `f'(x)`; exact for polynomials up
 *         to degree 4.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
derivative_five_point(F&& f, T x, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "derivative_five_point requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(h != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::derivative_five_point: step must be non-zero"),
                                  static_cast<T>(0));
    return (f(x - static_cast<T>(2) * h) - static_cast<T>(8) * f(x - h)
          + static_cast<T>(8) * f(x + h) - f(x + static_cast<T>(2) * h)) / (static_cast<T>(12) * h);
}

/**
 * @brief Richardson-extrapolated central difference.
 * @tparam Levels Number of step halvings plus one, in `[2, 16]`; uses
 *         `2 * Levels` evaluations of `f`.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `x`.
 * @param f Function to differentiate.
 * @param x Evaluation point.
 * @param h Non-zero initial (largest) step.
 * @return Estimate of `f'(x)` with error `O(h^(2 * Levels))` for smooth `f`.
 */
template <size_type Levels = 4U, typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
derivative_richardson(F&& f, T x, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "derivative_richardson requires a callable T f(T)");
    static_assert((Levels >= 2U) && (Levels <= 16U), "derivative_richardson: Levels must be in [2, 16]");
    CASTLE_ASSERT_OR_RETURN_VALUE(h != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::derivative_richardson: step must be non-zero"),
                                  static_cast<T>(0));

    // Extrapolation tableau kept in one row: table[m] holds D(level, m).
    T table[Levels] = {};
    T step = h;
    for (size_type level = 0U; level < Levels; ++level)
    {
        T diagonal_prev = table[0];
        table[0] = (f(x + step) - f(x - step)) / (static_cast<T>(2) * step);

        T factor = static_cast<T>(1);
        for (size_type m = 1U; m <= level; ++m)
        {
            factor *= static_cast<T>(4);
            CASTLE_CONST T improved = table[m - 1U] + (table[m - 1U] - diagonal_prev) / (factor - static_cast<T>(1));
            diagonal_prev = table[m];
            table[m] = improved;
        }
        step *= static_cast<T>(0.5);
    }
    return table[Levels - 1U];
}

/**
 * @brief Second derivative `(f(x + h) - 2f(x) + f(x - h)) / h^2`.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `x`.
 * @param f Function to differentiate.
 * @param x Evaluation point.
 * @param h Non-zero step; rounding noise grows as `1 / h^2`, so avoid very small values.
 * @return Second-order accurate estimate of `f''(x)`.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
second_derivative(F&& f, T x, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "second_derivative requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(h != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::second_derivative: step must be non-zero"),
                                  static_cast<T>(0));
    return (f(x + h) - static_cast<T>(2) * f(x) + f(x - h)) / (h * h);
}

/**
 * @brief Differentiates uniformly spaced samples.
 * @tparam T Floating-point sample type.
 * @param y Samples `y[0..count-1]`.
 * @param count Number of samples, at least 2.
 * @param dx Non-zero spacing between consecutive samples.
 * @param out Destination with room for `count` values; may alias `y`.
 * @return `status::ok` on success; `status::invalid_argument` for null
 *         pointers, fewer than two samples or a zero spacing.
 * @details Interior points use the central difference; the two end points use
 *          the one-sided three-point formulas, so all points are second-order
 *          accurate when `count >= 3`. With exactly two samples the result is
 *          the forward difference everywhere. `O(count)`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, status>::type
derivative_samples(CASTLE_CONST T* y, size_type count, meta::type_identity_t<T> dx, T* out) CASTLE_NOEXCEPT
{
    if ((y == nullptr) || (out == nullptr) || (count < 2U) || (dx == static_cast<T>(0)))
    {
        return status::invalid_argument;
    }

    if (count == 2U)
    {
        CASTLE_CONST T slope = (y[1] - y[0]) / dx;
        out[0] = slope;
        out[1] = slope;
        return status::ok;
    }

    // Capture the end samples first so that `out` may alias `y`.
    CASTLE_CONST T first = (-static_cast<T>(3) * y[0] + static_cast<T>(4) * y[1] - y[2]) / (static_cast<T>(2) * dx);
    CASTLE_CONST T last = (static_cast<T>(3) * y[count - 1U] - static_cast<T>(4) * y[count - 2U] + y[count - 3U])
                          / (static_cast<T>(2) * dx);

    T previous = y[0];
    for (size_type i = 1U; i + 1U < count; ++i)
    {
        CASTLE_CONST T current = y[i];
        out[i] = (y[i + 1U] - previous) / (static_cast<T>(2) * dx);
        previous = current;
    }
    out[0] = first;
    out[count - 1U] = last;
    return status::ok;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_NUMERIC_DIFFERENTIATION_HPP
