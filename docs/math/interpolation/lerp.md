# Linear Interpolation

## Overview
Computes the straight-line interpolation formula `a + t * (b - a)` for floating-point values. It exists as a tiny deterministic helper for interpolation and extrapolation without pulling in the STL. `linear_interpolate()` applies the same formula to a sampled curve.

## Header
`#include "castle/math/interpolation/lerp.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)
- [`castle/core/types.hpp`](../../core/types.md)
- [`castle/math/interpolation/knots.hpp`](knots.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T lerp(T a, T b, T t) noexcept` | Returns `a + t * (b - a)` for floating-point `T`. `t == 0` yields `a`, `t == 1` yields `b`, and values outside `[0, 1]` extrapolate. `O(1)`. |
| `template <typename T> T linear_interpolate(const T* xs, const T* ys, size_type count, T x) noexcept` | Interpolates the sampled curve `(xs[i], ys[i])` at `x`. `xs` must be strictly increasing. Clamps to `ys[0]` and `ys[count - 1]` outside the range; returns zero for null pointers or `count == 0`. `O(log count)`. |

## Usage Example
```cpp
#include "castle/math/interpolation/lerp.hpp"

constexpr float midpoint = castle::math::lerp(10.0f, 20.0f, 0.5f);
constexpr float future = castle::math::lerp(10.0f, 20.0f, 1.5f);

const float xs[] = {0.0F, 1.0F, 2.0F};
const float ys[] = {0.0F, 2.0F, 8.0F};
const float sampled = castle::math::linear_interpolate(xs, ys, 3U, 1.5F); // 5
```

See [`samples/math/interpolation/lerp.cpp`](../../../samples/math/interpolation/lerp.cpp).

## Constraints & Notes
- `t` is intentionally not clamped.
- Only floating-point types participate; integral types are excluded to avoid silently discarding fractional interpolation.
- This direct formula can lose precision or overflow more readily than a numerically specialized interpolation routine for extreme input ranges.
