# Plane3D

## Overview
A 3D plane stored as one point plus a normal vector. Use it when you need plane membership tests or supporting surfaces for intersection calculations.

## Header
`#include "castle/math/geometry/plane3d.hpp"`

## Dependencies
- [`detail.md`](detail.md)
- [`vector3d.md`](vector3d.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/error_handler.md`](../../core/error_handler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `plane3d()` | Constructs a degenerate plane with zero normal. |
| `plane3d(point, normal)` | Constructs a plane from one point and a non-zero normal. |
| `point()` / `normal()` | Return the stored point and normal. |
| `signed_value(value)` | Returns `normal dot (value - point())`. |
| `contains(value, epsilon)` | Returns true when `signed_value(value)` is within `epsilon` of zero. |

## Usage Example
See `samples/sample_plane3d.cpp`.

```cpp
#include "castle/math/geometry/plane3d.hpp"

castle::math::plane3d<int> plane(
    castle::math::point3d<int>(0, 0, 1),
    castle::math::vector3d<int>(0, 0, 2));
```

## Constraints & Notes
- The normal is asserted non-zero in the explicit constructor.
- `signed_value` is not a metric distance unless the stored normal has unit length.
- `contains` uses an absolute tolerance on that raw signed value.
