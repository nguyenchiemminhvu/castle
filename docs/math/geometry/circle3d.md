# Circle3D

## Overview
A 3D circle represented by a center, an explicit plane normal, and a radius. Use it for planar disks and circumferences embedded in world space.

## Header
`#include "castle/math/geometry/circle3d.hpp"`

## Dependencies
- [`detail.md`](detail.md)
- [`point3d.md`](point3d.md)
- [`plane3d.md`](plane3d.md)
- [`vector3d.md`](vector3d.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/error_handler.md`](../../core/error_handler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `circle3d()` | Constructs a zero-radius circle centered at the origin with default normal `(0, 0, 1)`. |
| `circle3d(center, normal, radius)` | Constructs a circle with a non-zero normal and non-negative floating-point radius. |
| `center()` / `normal()` / `radius()` | Access the stored geometry. |
| `contains(point, epsilon)` | Tests whether a point lies in the closed circular disk within plane/radial tolerance. |
| `on_circle(point, epsilon)` | Tests whether a point lies on the circumference within plane/radial tolerance. |
| `plane()` | Returns the supporting `plane3d`. |

## Usage Example
See `samples/sample_circle3d.cpp`.

```cpp
#include "castle/math/geometry/circle3d.hpp"

castle::math::circle3d<float> c(
    castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
    castle::math::vector3d<float>(0.0f, 0.0f, 2.0f),
    5.0f);
```

## Constraints & Notes
- Only floating-point coordinate types are accepted.
- The normal is asserted non-zero but is not normalized automatically.
- Plane tolerance scales with the squared normal length because the implementation works directly with the stored normal.
- `contains` checks the filled disk; `on_circle` checks the circumference.
