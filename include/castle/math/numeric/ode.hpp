// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ode.hpp
 * @brief Single-step explicit solvers for ordinary differential equations.
 * @details `euler_step` (first order) and `rk4_step` (classical fourth-order
 * Runge-Kutta) advance `y' = f(t, y)` by one step of size `h`. The state type
 * `S` is generic: any type with `S + S` and `S * T` works, so the same code
 * integrates a scalar, a `castle::math::vector<T, N>` or a `complex<T>`. The
 * caller owns the time loop, which keeps stack usage constant and lets the
 * application choose fixed steps, logging and termination conditions.
 *
 * @code
 * #include "castle/math/numeric/ode.hpp"
 *
 * // y' = -y, y(0) = 1  ->  y(1) = e^-1
 * auto f = [](double, double y) { return -y; };
 * double y = 1.0;
 * for (int i = 0; i < 10; ++i)
 * {
 *     y = castle::math::rk4_step(f, 0.1 * i, y, 0.1);
 * }
 * @endcode
 *
 * @note The callable is invoked as `f(t, y)` and must return `S`; it must be
 *       non-throwing. Each step costs 1 (Euler) or 4 (RK4) evaluations.
 */
#ifndef CASTLE_MATH_NUMERIC_ODE_HPP
#define CASTLE_MATH_NUMERIC_ODE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Advances `y' = f(t, y)` by one explicit Euler step.
 * @tparam F Callable type, invoked as `S f(T, S)`.
 * @tparam T Floating-point time type, deduced from `t`.
 * @tparam S State type supporting `S + S` and `S * T`.
 * @param f Right-hand side.
 * @param t Current time.
 * @param y Current state.
 * @param h Step size (may be negative to integrate backwards).
 * @return `y + h * f(t, y)`; local error `O(h^2)`.
 */
template <typename F, typename T, typename S>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, S>::type
euler_step(F&& f, T t, CASTLE_CONST S& y, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T, S>::value, "euler_step requires a callable S f(T, S)");
    return y + f(t, y) * h;
}

/**
 * @brief Advances `y' = f(t, y)` by one classical Runge-Kutta (RK4) step.
 * @tparam F Callable type, invoked as `S f(T, S)`.
 * @tparam T Floating-point time type, deduced from `t`.
 * @tparam S State type supporting `S + S` and `S * T`.
 * @param f Right-hand side.
 * @param t Current time.
 * @param y Current state.
 * @param h Step size (may be negative to integrate backwards).
 * @return The state at `t + h`; local error `O(h^5)`.
 */
template <typename F, typename T, typename S>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, S>::type
rk4_step(F&& f, T t, CASTLE_CONST S& y, meta::type_identity_t<T> h) CASTLE_NOEXCEPT
{
    static_assert(meta::is_invocable<F&, T, S>::value, "rk4_step requires a callable S f(T, S)");
    CASTLE_CONST T half = h * static_cast<T>(0.5);
    CASTLE_CONST S k1 = f(t, y);
    CASTLE_CONST S k2 = f(t + half, y + k1 * half);
    CASTLE_CONST S k3 = f(t + half, y + k2 * half);
    CASTLE_CONST S k4 = f(t + h, y + k3 * h);
    return y + (k1 + (k2 + k3) * static_cast<T>(2) + k4) * (h / static_cast<T>(6));
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_NUMERIC_ODE_HPP
