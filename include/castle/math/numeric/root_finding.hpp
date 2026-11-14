// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file root_finding.hpp
 * @brief Scalar root finders: bisection, Newton-Raphson and secant.
 * @details Each routine solves `f(x) = 0` for a callable `T f(T)` and reports
 * its outcome through `numeric_result<T>`, so no exceptions or out-parameters
 * are needed:
 *  - `find_root_bisection`: bracketing, guaranteed convergence, linear rate.
 *  - `find_root_newton`: needs the derivative, quadratic rate near a simple root.
 *  - `find_root_secant`: derivative-free, super-linear rate.
 *
 * Convergence means the error estimate (half bracket width for bisection, last
 * step length for Newton and secant) satisfies
 * `error <= tolerance * max(1, |x|)`. All loops are bounded by the caller's
 * `max_iterations`, which keeps worst-case execution time deterministic.
 *
 * @code
 * #include "castle/math/numeric/root_finding.hpp"
 *
 * auto f = [](double x) { return x * x - 2.0; };
 * const auto r = castle::math::find_root_bisection(f, 0.0, 2.0, 1e-12);
 * if (r.ok())
 * {
 *     // r.value ~= sqrt(2)
 * }
 * @endcode
 *
 * @note Callables are invoked as `f(x)` and must be non-throwing.
 */
#ifndef CASTLE_MATH_NUMERIC_ROOT_FINDING_HPP
#define CASTLE_MATH_NUMERIC_ROOT_FINDING_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/status.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/numeric/result.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Finds a root inside a sign-changing bracket by interval halving.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `lo`.
 * @param f Function whose root is sought.
 * @param lo One end of the bracket.
 * @param hi The other end of the bracket (either order is accepted).
 * @param tolerance Convergence tolerance on the half bracket width.
 * @param max_iterations Iteration budget.
 * @return `numeric_result` with the root estimate in `value` and the half
 *         bracket width in `error`. `code` is `status::ok` on convergence
 *         (including an exact zero at an end point or midpoint),
 *         `status::invalid_argument` when `f(lo)` and `f(hi)` have the same
 *         strict sign, and `status::out_of_range` when the budget was exhausted.
 *         One evaluation of `f` per iteration plus two for the bracket.
 */
template <typename F, typename T>
CASTLE_NODISCARD
numeric_result<typename meta::enable_if<meta::is_floating_point<T>::value, T>::type>
find_root_bisection(F&& f,
                    T lo,
                    meta::type_identity_t<T> hi,
                    meta::type_identity_t<T> tolerance,
                    unsigned max_iterations = 100U) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "find_root_bisection requires a callable T f(T)");

    T f_lo = f(lo);
    CASTLE_CONST T f_hi = f(hi);
    if (f_lo == static_cast<T>(0))
    {
        return numeric_result<T>{lo, static_cast<T>(0), 0U, status::ok};
    }
    if (f_hi == static_cast<T>(0))
    {
        return numeric_result<T>{hi, static_cast<T>(0), 0U, status::ok};
    }
    if ((f_lo < static_cast<T>(0)) == (f_hi < static_cast<T>(0)))
    {
        return numeric_result<T>{lo, castle::math::abs(hi - lo), 0U, status::invalid_argument};
    }

    numeric_result<T> result = {lo, castle::math::abs(hi - lo), 0U, status::out_of_range};
    for (unsigned i = 1U; i <= max_iterations; ++i)
    {
        CASTLE_CONST T mid = lo + static_cast<T>(0.5) * (hi - lo);
        CASTLE_CONST T f_mid = f(mid);
        if (f_mid == static_cast<T>(0))
        {
            return numeric_result<T>{mid, static_cast<T>(0), i, status::ok};
        }

        if ((f_mid < static_cast<T>(0)) == (f_lo < static_cast<T>(0)))
        {
            lo = mid;
            f_lo = f_mid;
        }
        else
        {
            hi = mid;
        }

        result.value = lo + static_cast<T>(0.5) * (hi - lo);
        result.error = static_cast<T>(0.5) * castle::math::abs(hi - lo);
        result.iterations = i;
        if (detail::within_tolerance(result.error, result.value, tolerance))
        {
            result.code = status::ok;
            return result;
        }
    }
    return result;
}

/**
 * @brief Finds a root with Newton-Raphson iteration.
 * @tparam F Callable type for the function, invoked as `T f(T)`.
 * @tparam DF Callable type for its derivative, invoked as `T df(T)`.
 * @tparam T Floating-point type, deduced from `x0`.
 * @param f Function whose root is sought.
 * @param df Derivative of `f`.
 * @param x0 Initial guess.
 * @param tolerance Convergence tolerance on the step length.
 * @param max_iterations Iteration budget.
 * @return `numeric_result` with the root estimate in `value` and the last step
 *         length in `error`. `code` is `status::ok` on convergence,
 *         `status::invalid_argument` when the derivative vanishes at an
 *         iterate, and `status::out_of_range` when the budget was exhausted.
 *         One evaluation of `f` and `df` per iteration.
 * @note Convergence is only guaranteed for starting points close enough to a
 *       simple root; prefer `find_root_bisection` when a bracket is known.
 */
template <typename F, typename DF, typename T>
CASTLE_NODISCARD
numeric_result<typename meta::enable_if<meta::is_floating_point<T>::value, T>::type>
find_root_newton(F&& f,
                 DF&& df,
                 T x0,
                 meta::type_identity_t<T> tolerance,
                 unsigned max_iterations = 50U) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "find_root_newton requires a callable T f(T)");
    static_assert(meta::is_invocable<DF&, T>::value, "find_root_newton requires a callable T df(T)");

    numeric_result<T> result = {x0, static_cast<T>(0), 0U, status::out_of_range};
    for (unsigned i = 1U; i <= max_iterations; ++i)
    {
        CASTLE_CONST T slope = df(result.value);
        if (slope == static_cast<T>(0))
        {
            result.code = status::invalid_argument;
            return result;
        }

        CASTLE_CONST T step = f(result.value) / slope;
        result.value -= step;
        result.error = castle::math::abs(step);
        result.iterations = i;
        if (detail::within_tolerance(result.error, result.value, tolerance))
        {
            result.code = status::ok;
            return result;
        }
    }
    return result;
}

/**
 * @brief Finds a root with the derivative-free secant method.
 * @tparam F Callable type, invoked as `T f(T)`.
 * @tparam T Floating-point type, deduced from `x0`.
 * @param f Function whose root is sought.
 * @param x0 First starting point.
 * @param x1 Second starting point, different from `x0`.
 * @param tolerance Convergence tolerance on the step length.
 * @param max_iterations Iteration budget.
 * @return `numeric_result` with the root estimate in `value` and the last step
 *         length in `error`. `code` is `status::ok` on convergence (including
 *         an exact zero), `status::invalid_argument` when two successive
 *         iterates give equal non-zero function values (flat secant), and
 *         `status::out_of_range` when the budget was exhausted. One evaluation
 *         of `f` per iteration plus two to start.
 */
template <typename F, typename T>
CASTLE_NODISCARD
numeric_result<typename meta::enable_if<meta::is_floating_point<T>::value, T>::type>
find_root_secant(F&& f,
                 T x0,
                 meta::type_identity_t<T> x1,
                 meta::type_identity_t<T> tolerance,
                 unsigned max_iterations = 50U) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T>::value, "find_root_secant requires a callable T f(T)");

    T f0 = f(x0);
    T f1 = f(x1);
    numeric_result<T> result = {x1, castle::math::abs(x1 - x0), 0U, status::out_of_range};
    for (unsigned i = 1U; i <= max_iterations; ++i)
    {
        if (f1 == static_cast<T>(0))
        {
            result.error = static_cast<T>(0);
            result.code = status::ok;
            return result;
        }
        if (f1 == f0)
        {
            result.code = status::invalid_argument;
            return result;
        }

        CASTLE_CONST T next = x1 - f1 * (x1 - x0) / (f1 - f0);
        result.error = castle::math::abs(next - x1);
        result.value = next;
        result.iterations = i;

        x0 = x1;
        f0 = f1;
        x1 = next;
        f1 = f(x1);
        if (detail::within_tolerance(result.error, result.value, tolerance))
        {
            result.code = status::ok;
            return result;
        }
    }
    return result;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_NUMERIC_ROOT_FINDING_HPP
