# Line3D

## Overview
An infinite 3D parametric line stored as origin plus direction. Use it for spatial projections, plane intersections, and point generation along a deterministic parameterization.

## Header
`#include "castle/math/geometry/line3d.hpp"`

## Dependencies
- [`vector3d.md`](vector3d.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `line3d()` | Constructs a degenerate line at the origin with zero direction. |
| `line3d(origin, direction)` | Constructs a line from an origin and an explicit direction vector. |
| `line3d(a, b)` | Constructs a line through two points with stored direction `b - a`. |
| `origin()` / `direction()` | Return the stored origin and direction. |
| `point_at(parameter)` | Evaluates `origin + direction * parameter`. |
| `degenerate()` | Returns true when the stored direction is exactly zero. |

## Usage Example
See `samples/sample_line3d.cpp`.

```cpp
#include "castle/math/geometry/line3d.hpp"

castle::math::line3d<float> axis(
    castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
    castle::math::vector3d<float>(0.0f, 0.0f, 2.0f));
```

## Constraints & Notes
- The direction is stored exactly as supplied; no normalization occurs.
- `degenerate()` uses exact zero comparisons only.
- The type is suitable for deterministic embedded geometry math.
