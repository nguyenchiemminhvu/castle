# Euclidean Norm

## Overview
Computes two-dimensional and three-dimensional floating-point vector magnitudes with scale normalization. It exists to reduce intermediate overflow risk compared with directly squaring large coordinates before calling `sqrt_real()`.

## Header
`#include "castle/math/hypot.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/math/abs.hpp`](abs.md)
- [`castle/math/sqrt_real.hpp`](sqrt_real.md)
- [`castle/algorithm/algorithm.hpp`](../algorithm/algorithm.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T hypot(T x, T y) noexcept` | Returns `sqrt(x * x + y * y)` for floating-point `T` after normalizing by the largest component magnitude. `O(1)`. |
| `template <typename T> T hypot(T x, T y, T z) noexcept` | Returns `sqrt(x * x + y * y + z * z)` for floating-point `T` after the same normalization step. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/hypot.hpp"

constexpr float planar = castle::math::hypot(3.0f, 4.0f);
constexpr float spatial = castle::math::hypot(1.0f, 2.0f, 2.0f);
```

See [`samples/sample_hypot.cpp`](../../samples/sample_hypot.cpp).

## Constraints & Notes
- Returns `0` when every component compares equal to zero.
- Scaling reduces overflow risk in the squaring step, but the final multiplication can still overflow if the true norm is not representable.
- Non-finite inputs are not specially handled beyond the behavior inherited from comparison, division, and `sqrt_real()`.
