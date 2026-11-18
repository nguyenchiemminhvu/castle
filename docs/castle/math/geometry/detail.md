# Geometry detail helpers

## Overview
Internal helper utilities shared by Castle geometry headers. Use this header only when you explicitly need the low-level widening alias or primitive determinant/dot helpers that back the public geometry APIs.

## Header
`#include "castle/math/geometry/detail.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)
- [`../abs.md`](../abs.md)
- [`../sqrt_real.md`](../sqrt_real.md)

## Public API
| API | Description |
|---|---|
| `detail::geometry_calc_type<T>` | Promotes common small integral coordinate types to `int64_t` for intermediate calculations. |
| `detail::cross2(ax, ay, bx, by)` | Signed 2D determinant `ax * by - ay * bx`. |
| `detail::dot2(ax, ay, bx, by)` | 2D dot product. |
| `detail::dot3(ax, ay, az, bx, by, bz)` | 3D dot product. |
| `detail::cross3_x(...)`, `cross3_y(...)`, `cross3_z(...)` | Individual components of a 3D cross product. |
| `detail::near_zero(value, epsilon)` | Absolute near-zero test using `castle::math::abs`. |

## Usage Example
See `samples/sample_detail.cpp`.

```cpp
#include "castle/math/geometry/detail.hpp"

using calc_t = castle::math::detail::geometry_calc_type<int>;
static_assert(sizeof(calc_t) >= sizeof(long long), "widened intermediate expected");
```

## Constraints & Notes
- Everything lives in `castle::math::detail` and is intended as supporting infrastructure.
- `geometry_calc_type` widens only integral types whose width is at most `int32_t`.
- The helper functions keep the caller-supplied arithmetic type for their return values.
