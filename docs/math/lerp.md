# Linear Interpolation

## Overview
Computes the straight-line interpolation formula `a + t * (b - a)` for floating-point values. It exists as a tiny deterministic helper for interpolation and extrapolation without pulling in the STL.

## Header
`#include "castle/math/lerp.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T lerp(T a, T b, T t) noexcept` | Returns `a + t * (b - a)` for floating-point `T`. `t == 0` yields `a`, `t == 1` yields `b`, and values outside `[0, 1]` extrapolate. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/lerp.hpp"

constexpr float midpoint = castle::math::lerp(10.0f, 20.0f, 0.5f);
constexpr float future = castle::math::lerp(10.0f, 20.0f, 1.5f);
```

See [`samples/sample_lerp.cpp`](../../samples/sample_lerp.cpp).

## Constraints & Notes
- `t` is intentionally not clamped.
- Only floating-point types participate; integral types are excluded to avoid silently discarding fractional interpolation.
- This direct formula can lose precision or overflow more readily than a numerically specialized interpolation routine for extreme input ranges.
