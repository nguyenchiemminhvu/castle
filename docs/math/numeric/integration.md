# Numerical Integration

## Overview
Deterministic, allocation-free numerical integration of functions and sampled data. Function integrators accept any callable `T f(T)` (function pointer, lambda, functor, `castle::function`, delegate): composite trapezoid, composite Simpson, composite 5-point Gauss-Legendre and tolerance-driven Romberg extrapolation. Sample integrators work on uniformly spaced data: trapezoid, Simpson (with a Simpson 3/8 closing panel for an odd number of intervals) and a cumulative trapezoid that writes the running integral to a buffer.

## Header
`#include "castle/math/numeric/integration.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/error_handler.hpp`](../../core/error_handler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)
- [`castle/core/types.hpp`](../../core/types.md)
- [`castle/error/status.hpp`](../../error/status.md)
- [`castle/math/abs.hpp`](../abs.md)
- [`castle/math/numeric/result.hpp`](result.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename F, typename T> constexpr T integrate_trapezoid(F&& f, T a, T b, size_type intervals) noexcept` | Composite trapezoid rule over `[a, b]` with `intervals` (at least 1) equal sub-intervals; second-order accurate. Exact for linear integrands. `O(intervals)` evaluations of `f`. Zero `intervals` triggers the Castle error handler and returns zero. |
| `template <typename F, typename T> constexpr T integrate_simpson(F&& f, T a, T b, size_type intervals) noexcept` | Composite Simpson rule; `intervals` (at least 1) is rounded up to the next even number. Fourth-order accurate, exact for cubics. `O(intervals)` evaluations. Zero `intervals` triggers the error handler and returns zero. |
| `template <typename F, typename T> constexpr T integrate_gauss_legendre(F&& f, T a, T b, size_type panels = 1U) noexcept` | Composite 5-point Gauss-Legendre rule with `panels` (at least 1) equal panels; each panel is exact up to degree 9. `5 * panels` evaluations, none at the limits. Zero `panels` triggers the error handler and returns zero. |
| `template <size_type MaxLevels = 12U, typename F, typename T> numeric_result<T> integrate_romberg(F&& f, T a, T b, T tolerance) noexcept` | Romberg integration (trapezoid refinement plus Richardson extrapolation). `MaxLevels` in `[2, 30]`, at most `2^(MaxLevels-1) + 1` evaluations, `2 * MaxLevels` scalars of stack. Returns `value` (integral), `error` (change between the last two diagonal estimates), `iterations` (levels used) and `code`: `status::ok` when `error <= tolerance * max(1, abs(value))`, otherwise `status::out_of_range` with the best estimate. |
| `template <typename T> constexpr T integrate_trapezoid_samples(const T* y, size_type count, T dx) noexcept` | Trapezoid rule for uniformly spaced samples `y[0..count-1]`; integrates over `(count - 1) * dx`. Returns zero for fewer than two samples. `O(count)`. |
| `template <typename T> constexpr T integrate_simpson_samples(const T* y, size_type count, T dx) noexcept` | Simpson rule for uniformly spaced samples. Even interval count: plain Simpson; odd interval count: Simpson over the even prefix plus one Simpson 3/8 panel over the last three intervals; a single interval falls back to the trapezoid rule. Returns zero for fewer than two samples. `O(count)`. |
| `template <typename T> constexpr status integrate_cumulative_trapezoid(const T* y, size_type count, T dx, T* out) noexcept` | Writes the running trapezoid integral to `out`: `out[0] = 0`, `out[i]` is the integral from sample 0 to sample `i`. `out` needs room for `count` values and may alias `y`. Returns `status::ok`, `status::invalid_argument` when `y` or `out` is null, or `status::empty` when `count` is zero. `O(count)`. |

All function templates participate only for floating-point `T`; `T` is deduced from the first limit (or the sample pointer), the remaining `T` parameters are non-deduced.

## Usage Example
```cpp
#include "castle/math/numeric/integration.hpp"

const double area = castle::math::integrate_simpson(
    [](double x) { return x * x; }, 0.0, 3.0, 100U); // 9

const auto r = castle::math::integrate_romberg(
    [](double x) { return x * x * x; }, 0.0, 2.0, 1e-12);
// r.ok() == true, r.value == 4

const double y[] = {0.0, 1.0, 4.0, 9.0, 16.0};
const double samples = castle::math::integrate_simpson_samples(y, 5U, 1.0); // 64/3
```

See [`samples/math/numeric/integration.cpp`](../../../samples/math/numeric/integration.cpp).

## Constraints & Notes
- Callables are invoked as `f(x)` and must be non-throwing; they are checked at compile time with `meta::is_invocable`.
- Integration limits may be given in either order; swapping them negates the result (the function integrators). The sample integrators always integrate in the direction of increasing index; a negative `dx` negates the result.
- `intervals == 0` / `panels == 0` are precondition violations reported through `CASTLE_ASSERT`; they are not a recoverable error path.
- `integrate_romberg` is non-recursive and not `constexpr`. Smooth integrands are required for fast convergence. `MaxLevels` outside `[2, 30]` fails a `static_assert`.
- `integrate_simpson_samples` with exactly 3 intervals uses a single Simpson 3/8 panel and no Simpson prefix.
- `integrate_cumulative_trapezoid` reads each `y[i]` before writing `out[i]`, so in-place use (`out == y`) is safe.
