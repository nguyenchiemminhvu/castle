# ODE Single-Step Solvers

## Overview
Explicit single-step solvers for ordinary differential equations `y' = f(t, y)`: `euler_step` (first order) and `rk4_step` (classical fourth-order Runge-Kutta). The state type `S` is generic: any type with `S + S` and `S * T` works, so the same code integrates a scalar, a `castle::math::vector<T, N>` or a `castle::math::complex<T>`. The caller owns the time loop, which keeps stack usage constant and lets the application choose fixed steps, logging and termination conditions.

## Header
`#include "castle/math/numeric/ode.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename F, typename T, typename S> S euler_step(F&& f, T t, const S& y, T h) noexcept` | Advances `y' = f(t, y)` by one explicit Euler step and returns `y + f(t, y) * h`. `f` is invoked as `S f(T, S)`; `t` is the current time, `y` the current state and `h` the step size (may be negative to integrate backwards). Local error `O(h^2)`, global error `O(h)`. One evaluation of `f`; `constexpr` when `f` and the state operations are. |
| `template <typename F, typename T, typename S> S rk4_step(F&& f, T t, const S& y, T h) noexcept` | Advances `y' = f(t, y)` by one classical RK4 step and returns the state at `t + h`, computed as `y + (k1 + 2*k2 + 2*k3 + k4) * (h / 6)`. Same parameters as `euler_step`. Local error `O(h^5)`, global error `O(h^4)`. Four evaluations of `f`; `constexpr` when `f` and the state operations are. |

Both functions are `CASTLE_NODISCARD` and enabled only for floating-point time type `T` (deduced from `t`; `h` is a non-deduced parameter of type `T`). The state type `S` is deduced from `y`.

## Usage Example
```cpp
#include "castle/math/numeric/ode.hpp"

// y' = -y, y(0) = 1  ->  y(1) = e^-1
const auto f = [](double, double y) { return -y; };
double y = 1.0;
for (int i = 0; i < 10; ++i)
{
    y = castle::math::rk4_step(f, 0.1 * i, y, 0.1);
}
```

See [`samples/math/numeric/ode.cpp`](../../../samples/math/numeric/ode.cpp).

## Constraints & Notes
- The callable is invoked as `f(t, y)` and must return `S`; it must be non-throwing.
- The state type must provide `S + S` and `S * T` (the callable's result and intermediate states are combined only through these operators).
- No step-size control or error estimation is performed; choose `h` appropriately for the problem. Explicit methods can be unstable for stiff systems.
- Passing a negative `h` integrates backwards in time.
- No allocation, no recursion, no exceptions.
