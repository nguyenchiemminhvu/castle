# Scalar Root Finding

## Overview
Solves `f(x) = 0` for a floating-point callable `T f(T)` with three classic methods: bracketing bisection (guaranteed convergence), Newton-Raphson (needs the derivative, quadratic rate near a simple root) and the derivative-free secant method (super-linear rate). Every routine returns a [`numeric_result<T>`](result.md) so no exceptions or out-parameters are needed, and every loop is bounded by the caller's `max_iterations`, which keeps the worst-case execution time deterministic.

Convergence means the error estimate (half bracket width for bisection, last step length for Newton and secant) satisfies `error <= tolerance * max(1, |x|)`.

## Header
`#include "castle/math/numeric/root_finding.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)
- [`castle/error/status.hpp`](../../error/status.md)
- [`castle/math/abs.hpp`](../abs.md)
- [`castle/math/numeric/result.hpp`](result.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename F, typename T> numeric_result<T> find_root_bisection(F&& f, T lo, T hi, T tolerance, unsigned max_iterations = 100U) noexcept` | Finds a root inside the bracket `[lo, hi]` by interval halving; the bracket may be given in either order. `f` is invoked as `T f(T)`. Returns the root estimate in `value`, the half bracket width in `error` and the iterations used. `code` is `status::ok` on convergence (an exact zero at `lo`, `hi` or a midpoint also yields `ok` with `error == 0`), `status::invalid_argument` when `f(lo)` and `f(hi)` have the same strict sign (`value == lo`, `iterations == 0`), and `status::out_of_range` when the budget was exhausted (`value` holds the best midpoint). Two evaluations of `f` for the bracket plus one per iteration; `O(max_iterations)`. |
| `template <typename F, typename DF, typename T> numeric_result<T> find_root_newton(F&& f, DF&& df, T x0, T tolerance, unsigned max_iterations = 50U) noexcept` | Newton-Raphson iteration from the initial guess `x0`. `f` and `df` are invoked as `T f(T)` / `T df(T)`. Returns the root estimate in `value` and the last step length in `error`. `code` is `status::ok` on convergence, `status::invalid_argument` when the derivative is exactly zero at an iterate (`value` holds that iterate), and `status::out_of_range` when the budget was exhausted. One evaluation each of `f` and `df` per iteration; `O(max_iterations)`. |
| `template <typename F, typename T> numeric_result<T> find_root_secant(F&& f, T x0, T x1, T tolerance, unsigned max_iterations = 50U) noexcept` | Derivative-free secant iteration from the starting points `x0` and `x1`. Returns the root estimate in `value` and the last step length in `error`. `code` is `status::ok` on convergence (including `f(x1) == 0` at an iterate, reported with `error == 0`), `status::invalid_argument` when two successive iterates give equal non-zero function values (flat secant), and `status::out_of_range` when the budget was exhausted. Two evaluations of `f` to start plus one per iteration; `O(max_iterations)`. |

All three functions are `CASTLE_NODISCARD`, are enabled only for floating-point `T` (deduced from the first value argument; the other numeric arguments are non-deduced), and static-assert that the callables are invocable with `T`.

## Usage Example
```cpp
#include "castle/math/numeric/root_finding.hpp"

const auto f = [](double x) { return x * x - 2.0; };
const auto df = [](double x) { return 2.0 * x; };

const auto bisection = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);
const auto newton = castle::math::find_root_newton(f, df, 1.0, 1.0e-12);
const auto secant = castle::math::find_root_secant(f, 1.0, 2.0, 1.0e-12);
if (bisection.ok() && newton.ok() && secant.ok())
{
    // each .value is approximately sqrt(2)
}
```

See [`samples/math/numeric/root_finding.cpp`](../../../samples/math/numeric/root_finding.cpp).

## Constraints & Notes
- Callables must be non-throwing and are invoked as `f(x)`; function pointers, lambdas and functors are all accepted.
- Newton's method only converges for starting points close enough to a simple root; prefer `find_root_bisection` when a bracket is known.
- Bisection requires a sign change over the bracket: a zero at an end point is accepted immediately, but a bracket with equal strict signs returns `status::invalid_argument` even if it contains an even number of roots.
- A failure with `status::out_of_range` still carries the best estimate in `value`; `status::invalid_argument` results carry the last usable point and should not be trusted as a root.
- The tolerance is mixed absolute/relative (`tolerance * max(1, |x|)`), consistent with [`near_equal()`](../near_equal.md).
- No allocation, no recursion, no exceptions.
