// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file integration.hpp
 * @brief Numerical integration of functions and sampled data.
 * @details Two families of deterministic, allocation-free routines:
 *  - Function integrators that take any callable `T f(T)` (function pointer,
 *    lambda, functor, `castle::function`, delegate): composite trapezoid,
 *    composite Simpson, composite 5-point Gauss-Legendre and tolerance-driven
 *    Romberg extrapolation. They never recurse and use at most
 *    `2 * MaxLevels` scalars of stack.
 *  - Sample integrators for uniformly spaced data: trapezoid, Simpson
 *    (with a Simpson 3/8 closing panel for an odd number of intervals) and a
 *    cumulative trapezoid that writes the running integral to a buffer.
 *
 * @code
 * #include "castle/math/numeric/integration.hpp"
 *
 * const double area = castle::math::integrate_simpson(
 *     [](double x) { return x * x; }, 0.0, 3.0, 100U); // 9
 *
 * const auto r = castle::math::integrate_romberg(
 *     [](double x) { return x * x * x; }, 0.0, 2.0, 1e-12);
 * @endcode
 *
 * @note Callables are invoked as `f(x)` and must be non-throwing. Integration
 *       limits may be given in either order; swapping them negates the result.
 */
#ifndef CASTLE_MATH_NUMERIC_INTEGRATION_HPP
#define CASTLE_MATH_NUMERIC_INTEGRATION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/core/status.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/numeric/result.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Composite trapezoid rule for a callable.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `a`.
 * @param f Integrand.
 * @param a Lower limit.
 * @param b Upper limit.
 * @param intervals Number of equal sub-intervals, at least 1.
 * @return Approximation of the integral; second-order accurate. `O(intervals)`
 *         evaluations of `f`.
 * @warning `intervals` must be positive; zero triggers the Castle error
 *          handler and yields zero.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
integrate_trapezoid(F&& f, T a, meta::type_identity_t<T> b, size_type intervals) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "integrate_trapezoid requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(intervals > 0U, // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::integrate_trapezoid: intervals must be positive"),
                                  static_cast<T>(0));
    CASTLE_CONST T h = (b - a) / static_cast<T>(intervals);
    T sum = static_cast<T>(0.5) * (f(a) + f(b));
    for (size_type i = 1U; i < intervals; ++i)
    {
        sum += f(a + static_cast<T>(i) * h);
    }
    return sum * h;
}

/**
 * @brief Composite Simpson rule for a callable.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `a`.
 * @param f Integrand.
 * @param a Lower limit.
 * @param b Upper limit.
 * @param intervals Number of equal sub-intervals, at least 1; odd values are
 *        rounded up to the next even number.
 * @return Approximation of the integral; fourth-order accurate (exact for
 *         cubics). `O(intervals)` evaluations of `f`.
 * @warning `intervals` must be positive; zero triggers the Castle error
 *          handler and yields zero.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
integrate_simpson(F&& f, T a, meta::type_identity_t<T> b, size_type intervals) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "integrate_simpson requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(intervals > 0U, // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::integrate_simpson: intervals must be positive"),
                                  static_cast<T>(0));
    CASTLE_CONST size_type n = intervals + (intervals & 1U);
    CASTLE_CONST T h = (b - a) / static_cast<T>(n);
    T odd_sum = static_cast<T>(0);
    T even_sum = static_cast<T>(0);
    for (size_type i = 1U; i < n; ++i)
    {
        CASTLE_CONST T value = f(a + static_cast<T>(i) * h);
        if ((i & 1U) != 0U)
        {
            odd_sum += value;
        }
        else
        {
            even_sum += value;
        }
    }
    return (h / static_cast<T>(3)) * (f(a) + f(b) + static_cast<T>(4) * odd_sum + static_cast<T>(2) * even_sum);
}

/**
 * @brief Composite 5-point Gauss-Legendre rule for a callable.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `a`.
 * @param f Integrand.
 * @param a Lower limit.
 * @param b Upper limit.
 * @param panels Number of equal panels, at least 1.
 * @return Approximation of the integral; each panel is exact for polynomials
 *         up to degree 9. `5 * panels` evaluations of `f`, none at the limits.
 * @warning `panels` must be positive; zero triggers the Castle error handler
 *          and yields zero.
 */
template <typename F, typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
integrate_gauss_legendre(F&& f, T a, meta::type_identity_t<T> b, size_type panels = 1U) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "integrate_gauss_legendre requires a callable T f(T)");
    CASTLE_ASSERT_OR_RETURN_VALUE(panels > 0U, // LCOV_EXCL_BR_LINE
                                  CASTLE_ERROR_GENERIC("castle::math::integrate_gauss_legendre: panels must be positive"),
                                  static_cast<T>(0));
    CASTLE_CONST T node1 = static_cast<T>(0.5384693101056831L);
    CASTLE_CONST T node2 = static_cast<T>(0.9061798459386640L);
    CASTLE_CONST T weight0 = static_cast<T>(0.5688888888888889L);
    CASTLE_CONST T weight1 = static_cast<T>(0.4786286704993665L);
    CASTLE_CONST T weight2 = static_cast<T>(0.2369268850561891L);

    CASTLE_CONST T panel_width = (b - a) / static_cast<T>(panels);
    CASTLE_CONST T half = static_cast<T>(0.5) * panel_width;
    T total = static_cast<T>(0);
    for (size_type i = 0U; i < panels; ++i)
    {
        CASTLE_CONST T center = a + (static_cast<T>(i) + static_cast<T>(0.5)) * panel_width;
        total += weight0 * f(center)
               + weight1 * (f(center - half * node1) + f(center + half * node1))
               + weight2 * (f(center - half * node2) + f(center + half * node2));
    }
    return total * half;
}

/**
 * @brief Romberg integration: trapezoid refinement with Richardson extrapolation.
 * @tparam MaxLevels Maximum number of halvings plus one, in `[2, 30]`; at most
 *         `2^(MaxLevels - 1) + 1` evaluations of `f`.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `a`.
 * @param f Integrand; should be smooth on `[a, b]`.
 * @param a Lower limit.
 * @param b Upper limit.
 * @param tolerance Relative/absolute tolerance, applied as
 *        `tolerance * max(1, |value|)`.
 * @return `numeric_result` whose `value` is the integral, `error` the change
 *         between the last two diagonal estimates, and `iterations` the number
 *         of levels used. `code` is `status::ok` on convergence or
 *         `status::out_of_range` when `MaxLevels` was exhausted first (the
 *         best estimate is still returned).
 */
template <size_type MaxLevels = 12U, typename F, typename T>
CASTLE_NODISCARD
numeric_result<typename meta::enable_if<meta::is_floating_point<T>::value, T>::type>
integrate_romberg(F&& f, T a, meta::type_identity_t<T> b, meta::type_identity_t<T> tolerance) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "integrate_romberg requires a callable T f(T)");
    static_assert((MaxLevels >= 2U) && (MaxLevels <= 30U), "integrate_romberg: MaxLevels must be in [2, 30]");

    T previous[MaxLevels] = {};
    T current[MaxLevels] = {};
    T h = b - a;
    previous[0] = static_cast<T>(0.5) * h * (f(a) + f(b));

    numeric_result<T> result = {previous[0], static_cast<T>(0), 1U, status::out_of_range};
    for (size_type level = 1U; level < MaxLevels; ++level)
    {
        h *= static_cast<T>(0.5);
        T midpoint_sum = static_cast<T>(0);
        CASTLE_CONST size_type midpoints = static_cast<size_type>(1U) << (level - 1U);
        for (size_type j = 0U; j < midpoints; ++j)
        {
            midpoint_sum += f(a + static_cast<T>(2U * j + 1U) * h);
        }

        current[0] = static_cast<T>(0.5) * previous[0] + h * midpoint_sum;
        T factor = static_cast<T>(1);
        for (size_type m = 1U; m <= level; ++m)
        {
            factor *= static_cast<T>(4);
            current[m] = current[m - 1U] + (current[m - 1U] - previous[m - 1U]) / (factor - static_cast<T>(1));
        }

        result.value = current[level];
        result.error = castle::math::abs(current[level] - previous[level - 1U]);
        result.iterations = static_cast<unsigned>(level + 1U);
        if (detail::within_tolerance(result.error, result.value, tolerance))
        {
            result.code = status::ok;
            return result;
        }

        for (size_type m = 0U; m <= level; ++m)
        {
            previous[m] = current[m];
        }
    }
    return result;
}

/**
 * @brief Trapezoid rule for uniformly spaced samples.
 * @tparam T Floating-point sample type.
 * @param y Samples `y[0..count-1]`.
 * @param count Number of samples.
 * @param dx Spacing between consecutive samples.
 * @return The integral over `(count - 1) * dx`; zero for fewer than two
 *         samples. `O(count)`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
integrate_trapezoid_samples(CASTLE_CONST T* y, size_type count, meta::type_identity_t<T> dx) CASTLE_NOEXCEPT
{
    if (count < 2U)
    {
        return static_cast<T>(0);
    }

    T sum = static_cast<T>(0.5) * (y[0] + y[count - 1U]);
    for (size_type i = 1U; i + 1U < count; ++i)
    {
        sum += y[i];
    }
    return sum * dx;
}

/**
 * @brief Simpson rule for uniformly spaced samples.
 * @tparam T Floating-point sample type.
 * @param y Samples `y[0..count-1]`.
 * @param count Number of samples.
 * @param dx Spacing between consecutive samples.
 * @return The integral over `(count - 1) * dx`. An even number of intervals
 *         uses plain Simpson; an odd number closes with one Simpson 3/8 panel
 *         over the last three intervals; a single interval falls back to the
 *         trapezoid rule. Zero for fewer than two samples. `O(count)`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
integrate_simpson_samples(CASTLE_CONST T* y, size_type count, meta::type_identity_t<T> dx) CASTLE_NOEXCEPT
{
    if (count < 2U)
    {
        return static_cast<T>(0);
    }

    CASTLE_CONST size_type intervals = count - 1U;
    if (intervals == 1U)
    {
        return static_cast<T>(0.5) * (y[0] + y[1]) * dx;
    }

    // Plain Simpson over the even prefix; an odd remainder of three intervals uses Simpson 3/8.
    CASTLE_CONST bool odd = (intervals & 1U) != 0U;
    CASTLE_CONST size_type prefix = odd ? (intervals - 3U) : intervals;

    T total = static_cast<T>(0);
    if (prefix > 0U)
    {
        T sum = y[0] + y[prefix];
        for (size_type i = 1U; i < prefix; ++i)
        {
            sum += ((i & 1U) != 0U ? static_cast<T>(4) : static_cast<T>(2)) * y[i];
        }
        total = sum * dx / static_cast<T>(3);
    }
    if (odd)
    {
        CASTLE_CONST T tail = y[prefix] + static_cast<T>(3) * (y[prefix + 1U] + y[prefix + 2U]) + y[prefix + 3U];
        total += static_cast<T>(3) * dx / static_cast<T>(8) * tail;
    }
    return total;
}

/**
 * @brief Cumulative trapezoid integral of uniformly spaced samples.
 * @tparam T Floating-point sample type.
 * @param y Samples `y[0..count-1]`.
 * @param count Number of samples.
 * @param dx Spacing between consecutive samples.
 * @param out Destination with room for `count` values; `out[0]` is zero and
 *        `out[i]` the integral from sample 0 to sample `i`. May alias `y`.
 * @return `status::ok` on success; `status::invalid_argument` when `y` or
 *         `out` is null; `status::empty` when `count` is zero. `O(count)`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, status>::type
integrate_cumulative_trapezoid(CASTLE_CONST T* y, size_type count, meta::type_identity_t<T> dx, T* out) CASTLE_NOEXCEPT
{
    if ((y == nullptr) || (out == nullptr))
    {
        return status::invalid_argument;
    }
    if (count == 0U)
    {
        return status::empty;
    }

    T previous_sample = y[0];
    T running = static_cast<T>(0);
    out[0] = running;
    for (size_type i = 1U; i < count; ++i)
    {
        CASTLE_CONST T sample = y[i];
        running += static_cast<T>(0.5) * (previous_sample + sample) * dx;
        out[i] = running;
        previous_sample = sample;
    }
    return status::ok;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_NUMERIC_INTEGRATION_HPP
