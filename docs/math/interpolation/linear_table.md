# Linear Table

## Overview
`castle::math::linear_table<T>` is a non-owning piecewise-linear lookup table. It binds to caller-owned `xs` / `ys` arrays (often `const` data in flash) and evaluates the polyline through the points `(xs[i], ys[i])`. The abscissas are validated once when the table is bound; afterwards each query costs one `O(log n)` segment lookup plus a `lerp`. No storage is owned, nothing is allocated, and bad input is reported through `castle::status`.

## Header
`#include "castle/math/interpolation/linear_table.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/error_handler.hpp`](../../core/error_handler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)
- [`castle/core/types.hpp`](../../core/types.md)
- [`castle/error/status.hpp`](../../error/status.md)
- [`castle/math/interpolation/knots.hpp`](knots.md)
- [`castle/math/interpolation/lerp.hpp`](lerp.md)

## Public API
`T` must be a floating-point type (enforced by `static_assert`). All members are `constexpr` and `noexcept`.

| Signature | Description |
| --- | --- |
| `using value_type = T; using size_type = castle::size_type;` | Member type aliases. |
| `linear_table()` | Constructs an unbound table; `valid()` is `false`. |
| `linear_table(const T* xs, const T* ys, size_type count)` | Binds to the given knots via `assign()`. On invalid input the table stays unbound (check `valid()`, or use `assign()` to get the status). |
| `status assign(const T* xs, const T* ys, size_type count)` | Binds the table to new knots. `xs` must be strictly increasing; `ys` has the same length; `count >= 2`. Returns `status::ok`, or `status::invalid_argument` for a null `xs`/`ys`, `count < 2`, or non-increasing/NaN abscissas. On failure the table is left unbound. `O(count)`. |
| `bool valid() const` | `true` when bound to valid knots. |
| `size_type size() const` | Number of knots, or `0` when unbound. |
| `T x_min() const` | Smallest abscissa (`xs[0]`). Table must be valid. |
| `T x_max() const` | Largest abscissa (`xs[count - 1]`). Table must be valid. |
| `T evaluate(T x, extrapolation mode = extrapolation::clamp) const` | Interpolates at `x`. Inside `[x_min, x_max]` returns the polyline value. Outside: `clamp` returns the end ordinate, `linear` extends the end segment's line. `O(log count)`. |
| `T operator()(T x) const` | Equivalent to `evaluate(x)` with `extrapolation::clamp`. |
| `T derivative(T x, extrapolation mode = extrapolation::clamp) const` | Slope of the containing segment (the right segment at an interior knot; the last segment at `x_max`). Outside the table: `0` for `clamp`, the end segment's slope for `linear`. `O(log count)`. |

## Usage Example
```cpp
#include "castle/math/interpolation/linear_table.hpp"

static const float volts[] = {0.5F, 1.5F, 2.5F, 3.3F};
static const float celsius[] = {-20.0F, 0.0F, 40.0F, 85.0F};

const castle::math::linear_table<float> curve(volts, celsius, 4U);
if (curve.valid())
{
    const float temperature = curve.evaluate(2.0F);                                  // 20
    const float beyond = curve.evaluate(4.0F, castle::math::extrapolation::linear);  // extends last segment
}
```

See [`samples/math/interpolation/linear_table.cpp`](../../../samples/math/interpolation/linear_table.cpp).

## Constraints & Notes
- The referenced arrays must outlive the table and must not change while it is in use; the table stores only pointers.
- Calling `evaluate`, `derivative`, `x_min` or `x_max` on an unbound table is a precondition violation: it triggers the Castle error handler (assert) and yields zero.
- `derivative` is discontinuous at interior knots; it reports the slope of the segment to the right.
- Only floating-point value types are supported.
