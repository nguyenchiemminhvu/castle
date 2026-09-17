# Point2D

## Overview
A small 2D position type with explicit point semantics. Use it when you need to store locations separately from vectors so point/vector operations stay dimensionally meaningful.

## Header
`#include "castle/math/geometry/point2d.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `point2d()` | Constructs the origin `(0, 0)`. |
| `point2d(x, y)` | Constructs a point from coordinates. |
| `x()` / `y()` | Return the stored coordinates. |
| `set_x(value)` / `set_y(value)` | Replace one coordinate. |
| `operator==`, `operator!=` | Component-wise equality/inequality. |

## Usage Example
See `samples/sample_point2d.cpp`.

```cpp
#include "castle/math/geometry/point2d.hpp"

castle::math::point2d<int> p(3, 4);
p.set_x(5);
```

## Constraints & Notes
- Accepts arithmetic coordinate types only.
- Performs exact component-wise comparison with the underlying type's `operator==`.
- Stores coordinates inline with no allocation or STL dependency.
