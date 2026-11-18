# Circle2D

## Overview
A 2D circle stored as center plus radius. Use it for filled-disk containment, circumference tests, and planar collision or range checks.

## Header
`#include "castle/math/geometry/circle2d.hpp"`

## Dependencies
- [`point2d.md`](point2d.md)
- [`vector2d.md`](vector2d.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/error_handler.md`](../../core/error_handler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `circle2d()` | Constructs a zero-radius circle at the origin. |
| `circle2d(center, radius)` | Constructs a circle with a non-negative floating-point radius. |
| `center()` | Returns the stored center point. |
| `radius()` | Returns the stored radius. |
| `contains(point, epsilon)` | Tests whether a point lies in the closed disk expanded by `epsilon`. |
| `on_circle(point, epsilon)` | Tests whether a point lies within the circumference band `[radius - epsilon, radius + epsilon]`, clamped at zero on the inside. |

## Usage Example
See `samples/sample_circle2d.cpp`.

```cpp
#include "castle/math/geometry/circle2d.hpp"

castle::math::circle2d<float> c(castle::math::point2d<float>(1.0f, 2.0f), 5.0f);
bool inside = c.contains(castle::math::point2d<float>(4.0f, 6.0f));
```

## Constraints & Notes
- Only floating-point coordinate types are accepted.
- Radius is asserted non-negative.
- `contains` works on the filled disk; `on_circle` works on the circumference band.
- No normalization or dynamic allocation is involved.
