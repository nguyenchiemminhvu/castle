# Interpolation Knots

## Overview
Shared knot-table helpers for the table-based interpolators ([`linear_table`](linear_table.md), [`cubic_spline`](cubic_spline.md)). It defines the `extrapolation` policy enum and two internal helpers: validation that an abscissa array is strictly increasing, and an `O(log n)` segment lookup built on `castle::upper_bound`. Sharing one header keeps lookup and validation rules identical across interpolators.

## Header
`#include "castle/math/interpolation/knots.hpp"`

## Dependencies
- [`castle/dsa/algorithm.hpp`](../../dsa/algorithm.md)
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/types.hpp`](../../core/types.md)
- [`castle/core/status.hpp`](../../core/status.md)

## Public API
| Signature | Description |
| --- | --- |
| `enum class extrapolation : uint8_t { clamp, linear }` | Behaviour of an interpolator outside its knot range. `clamp` holds the nearest end value (slope zero); `linear` continues along the tangent line at the nearest end knot. |
| `template <typename T> status detail::validate_knots(const T* xs, size_type count) noexcept` | Returns `status::ok` when `xs` is non-null, `count >= 2` and `xs` is strictly increasing; otherwise `status::invalid_argument`. NaN entries fail the strict-increase test and are rejected. `O(count)`. |
| `template <typename T> size_type detail::find_segment(const T* xs, size_type count, T x) noexcept` | Returns the index `i` in `[0, count - 2]` with `xs[i] <= x < xs[i + 1]`. Queries below the table map to `0`; queries at or beyond the last knot map to `count - 2`. `xs` must be strictly increasing and `count >= 2`. `O(log count)`. |

## Usage Example
```cpp
#include "castle/math/interpolation/knots.hpp"

const float xs[] = {0.0F, 1.0F, 4.0F};
const castle::status s = castle::math::detail::validate_knots(xs, 3U);            // ok
const castle::size_type i = castle::math::detail::find_segment(xs, 3U, 2.0F);     // 1
const castle::math::extrapolation mode = castle::math::extrapolation::linear;
```

See [`samples/math/interpolation/knots.cpp`](../../../samples/math/interpolation/knots.cpp).

## Constraints & Notes
- `detail::` helpers are implementation details of the interpolation module; they are documented because both table classes depend on their exact contract.
- `find_segment` performs no validation; passing unsorted knots or `count < 2` is a precondition violation.
- A query exactly equal to an interior knot belongs to the segment on its right; a query exactly at the last knot belongs to the last segment.
