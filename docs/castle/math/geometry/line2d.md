# Line2D

## Overview
An infinite 2D parametric line stored as origin plus direction. Use it for projections, line relationships, and point generation along a deterministic parameterization.

## Header
`#include "castle/math/geometry/line2d.hpp"`

## Dependencies
- [`vector2d.md`](vector2d.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `line2d()` | Constructs a degenerate line at the origin with zero direction. |
| `line2d(origin, direction)` | Constructs a line from an origin and an explicit direction vector. |
| `line2d(a, b)` | Constructs a line through two points with stored direction `b - a`. |
| `origin()` / `direction()` | Return the stored origin and direction. |
| `point_at(parameter)` | Evaluates `origin + direction * parameter`. |
| `degenerate()` | Returns true when the stored direction is exactly zero. |

## Usage Example
See `samples/sample_line2d.cpp`.

```cpp
#include "castle/math/geometry/line2d.hpp"

castle::math::line2d<int> line(
    castle::math::point2d<int>(1, 1),
    castle::math::point2d<int>(4, 5));
```

## Constraints & Notes
- The direction is stored exactly as supplied; no normalization occurs.
- `degenerate()` uses exact zero comparisons only.
- The header itself does not allocate memory or depend on the STL.
