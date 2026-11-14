# Numerical Differentiation

## Overview
Finite-difference derivative estimators for any callable `T f(T)` and for uniformly spaced samples: one-sided forward and backward differences, second-order central difference, fourth-order five-point stencil, Richardson-extrapolated central difference, second derivative, and a gradient routine for sampled data. All routines are allocation-free and non-recursive; the step `h` is always supplied by the caller.

## Header
`#include "castle/math/numeric/differentiation.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/error_handler.hpp`](../../core/error_handler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)
- [`castle/core/types.hpp`](../../core/types.md)
- [`castle/core/status.hpp`](../../core/status.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename F, typename T> constexpr T derivative_forward(F&& f, T x, T h) noexcept` | Forward difference `(f(x + h) - f(x)) / h`; first-order accurate. `h` must be non-zero. `O(1)`, 2 evaluations. |
| `template <typename F, typename T> constexpr T derivative_backward(F&& f, T x, T h) noexcept` | Backward difference `(f(x) - f(x - h)) / h`; first-order accurate. `h` must be non-zero. `O(1)`, 2 evaluations. |
| `template <typename F, typename T> constexpr T derivative_central(F&& f, T x, T h) noexcept` | Central difference `(f(x + h) - f(x - h)) / (2h)`; second-order accurate (exact for quadratics). `h` must be non-zero. `O(1)`, 2 evaluations. |
| `template <typename F, typename T> constexpr T derivative_five_point(F&& f, T x, T h) noexcept` | Five-point stencil `(f(x-2h) - 8f(x-h) + 8f(x+h) - f(x+2h)) / (12h)`; fourth-order accurate, exact for polynomials up to degree 4. `h` must be non-zero. `O(1)`, 4 evaluations. |
| `template <size_type Levels = 4U, typename F, typename T> constexpr T derivative_richardson(F&& f, T x, T h) noexcept` | Richardson extrapolation of central differences with step `h, h/2, h/4, ...`. `Levels` in `[2, 16]`; error `O(h^(2 * Levels))` for smooth `f`. `h` must be non-zero. `2 * Levels` evaluations, `O(Levels^2)` arithmetic, `Levels` scalars of stack. |
| `template <typename F, typename T> constexpr T second_derivative(F&& f, T x, T h) noexcept` | Second derivative `(f(x + h) - 2f(x) + f(x - h)) / h^2`; second-order accurate. `h` must be non-zero; rounding noise grows as `1 / h^2`. `O(1)`, 3 evaluations. |
| `template <typename T> constexpr status derivative_samples(const T* y, size_type count, T dx, T* out) noexcept` | Gradient of uniformly spaced samples. Interior points use the central difference, the two end points the one-sided three-point formulas (second-order accurate for `count >= 3`); with exactly two samples both outputs equal the forward difference. `out` needs room for `count` values and may alias `y`. Returns `status::ok` or `status::invalid_argument` for a null `y`/`out`, `count < 2` or `dx == 0`. Negative `dx` is allowed. `O(count)`. |

All function templates participate only for floating-point `T`; `T` is deduced from `x` (or the sample pointer), `h`/`dx` are non-deduced.

## Usage Example
```cpp
#include "castle/math/numeric/differentiation.hpp"

auto f = [](double x) { return x * x * x; };
const double d1 = castle::math::derivative_central(f, 2.0, 1e-4);   // ~12
const double d2 = castle::math::second_derivative(f, 2.0, 1e-3);    // ~12
const double d3 = castle::math::derivative_richardson<5U>(f, 2.0, 0.5); // 12

double y[] = {0.0, 1.0, 4.0, 9.0, 16.0};
double dydx[5];
const castle::status s = castle::math::derivative_samples(y, 5U, 1.0, dydx); // dydx[i] == 2 * i
```

See [`samples/math/numeric/differentiation.cpp`](../../../samples/math/numeric/differentiation.cpp).

## Constraints & Notes
- Callables are invoked as `f(x)` and must be non-throwing; they are checked at compile time with `meta::is_invocable`.
- A zero step in the function-based estimators is a precondition violation reported through `CASTLE_ASSERT` (the call then returns zero); it is not a recoverable error path. Only `derivative_samples` reports bad input through `status`.
- Rule of thumb for the step: about `eps^(1/2) * max(1, |x|)` for one-sided differences and `eps^(1/3) * max(1, |x|)` for central differences. Very small steps amplify rounding noise, particularly in `second_derivative`.
- `derivative_richardson` fails a `static_assert` when `Levels` is outside `[2, 16]`; too many levels with a tiny initial `h` can degrade accuracy through cancellation.
- `derivative_samples` captures both end-point derivatives before overwriting any element and keeps the previous sample in a local, so in-place use (`out == y`) is safe.
