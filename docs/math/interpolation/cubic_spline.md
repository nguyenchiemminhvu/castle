# Cubic Spline

## Overview
`castle::math::cubic_spline<T, Capacity>` is a fixed-capacity cubic spline over tabulated data. It copies up to `Capacity` knots into inline storage and precomputes one tangent per knot, so every segment is a cubic Hermite polynomial (see [`curves`](curves.md)). Three tangent rules are selected through `spline_kind`: `natural` (C2, zero second derivative at both ends), `clamped` (C2, caller-supplied end slopes) and `monotone` (C1 shape-preserving PCHIP interpolant that never overshoots monotone data, suited to sensor calibration curves). Fitting is `O(n)`, each query is an `O(log n)` segment lookup plus a cubic evaluation; there is no heap and no exceptions.

## Header
`#include "castle/math/interpolation/cubic_spline.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/error_handler.hpp`](../../core/error_handler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)
- [`castle/core/types.hpp`](../../core/types.md)
- [`castle/error/status.hpp`](../../error/status.md)
- [`castle/math/abs.hpp`](../abs.md)
- [`castle/math/interpolation/curves.hpp`](curves.md)
- [`castle/math/interpolation/knots.hpp`](knots.md)
- [`castle/math/sign.hpp`](../sign.md)

## Public API
`T` must be a floating-point type and `Capacity >= 2` (both enforced by `static_assert`). All members are `constexpr` and `noexcept`.

| Signature | Description |
| --- | --- |
| `enum class spline_kind : uint8_t { natural, clamped, monotone }` | Tangent rule. `natural`: C2, zero second derivative at both ends. `clamped`: C2, end slopes supplied by the caller. `monotone`: C1 PCHIP (Fritsch-Carlson/Butland) shape-preserving interpolant. |
| `using value_type = T; using size_type = castle::size_type;` | Member type aliases. |
| `using index_type = meta::smallest_uint_t<Capacity>` | Compact storage type for the knot count. |
| `cubic_spline()` | Constructs an empty spline; `valid()` is `false`. |
| `static size_type capacity()` | Returns `Capacity`, the maximum number of knots. |
| `status build(const T* xs, const T* ys, size_type count, spline_kind kind = spline_kind::natural, T start_slope = 0, T end_slope = 0)` | Copies the knots and computes the tangents. `count` is in `[2, Capacity]`; `xs` strictly increasing. `start_slope` / `end_slope` are used by `spline_kind::clamped` only. Returns `status::ok`; `status::invalid_argument` for null pointers, `count < 2`, or non-increasing/NaN abscissas; `status::full` when `count > Capacity`. On any failure the spline is left empty (`valid() == false`), even if it previously held a fit. `O(count)`. |
| `bool valid() const` | `true` when the spline holds fitted knots. |
| `size_type size() const` | Number of knots, or `0` when empty. |
| `T x_min() const` | Smallest abscissa. Spline must be valid. |
| `T x_max() const` | Largest abscissa. Spline must be valid. |
| `T evaluate(T x, extrapolation mode = extrapolation::clamp) const` | Evaluates the spline. Inside `[x_min, x_max]` returns the Hermite cubic of the containing segment (reproducing the knots exactly). Outside: `clamp` returns the end ordinate, `linear` returns the tangent line at the nearest end knot (C1 across the boundary). `O(log count)`. |
| `T operator()(T x) const` | Equivalent to `evaluate(x)` with `extrapolation::clamp`. |
| `T derivative(T x, extrapolation mode = extrapolation::clamp) const` | First derivative `dy/dx`. Outside the table: `0` for `clamp`, the end-knot slope for `linear`. `O(log count)`. |

## Usage Example
```cpp
#include "castle/math/interpolation/cubic_spline.hpp"

const float xs[] = {0.0F, 1.0F, 2.0F, 3.0F};
const float ys[] = {0.0F, 1.0F, 8.0F, 27.0F};

castle::math::cubic_spline<float, 8U> spline;
if (spline.build(xs, ys, 4U, castle::math::spline_kind::clamped, 0.0F, 27.0F) == castle::status::ok)
{
    const float y = spline.evaluate(1.5F);
    const float slope = spline.derivative(1.5F);
    const float outside = spline.evaluate(4.0F, castle::math::extrapolation::linear);
}
```

See [`samples/math/interpolation/cubic_spline.cpp`](../../../samples/math/interpolation/cubic_spline.cpp).

## Constraints & Notes
- `build()` places `sizeof(T) * Capacity` bytes of scratch space on the stack for the tridiagonal (Thomas algorithm) solve of the `natural` and `clamped` kinds.
- The object itself holds `3 * Capacity` values of `T` plus the knot count; it is trivially copyable and needs no external storage after `build()` returns.
- Calling `evaluate`, `derivative`, `x_min` or `x_max` on an empty spline is a precondition violation: it triggers the Castle error handler (assert) and yields zero.
- `natural` and `clamped` splines are C2; `monotone` is only C1. A `monotone` spline stays within `[min, max]` of monotone data and is exactly flat across runs of equal ordinates; local extrema in the data get zero slope.
- With `spline_kind::monotone` the `start_slope` / `end_slope` arguments are ignored; end tangents come from a limited one-sided three-point formula.
- A `clamped` spline reproduces any cubic polynomial exactly (up to rounding) when the true end slopes are given.
- Two knots produce a straight line for `natural` and `monotone`, and a Hermite cubic with the given end slopes for `clamped`.
