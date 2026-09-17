# Point3D

## Overview
A small 3D position type with explicit point semantics. Use it when you need stable storage for spatial locations while keeping positions distinct from displacement vectors.

## Header
`#include "castle/math/geometry/point3d.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `point3d()` | Constructs the origin `(0, 0, 0)`. |
| `point3d(x, y, z)` | Constructs a point from coordinates. |
| `x()` / `y()` / `z()` | Return the stored coordinates. |
| `set_x(value)`, `set_y(value)`, `set_z(value)` | Replace one coordinate. |
| `operator==`, `operator!=` | Component-wise equality/inequality. |

## Usage Example
See `samples/sample_point3d.cpp`.

```cpp
#include "castle/math/geometry/point3d.hpp"

castle::math::point3d<int> p(1, 2, 3);
p.set_z(6);
```

## Constraints & Notes
- Accepts arithmetic coordinate types only.
- Performs exact component-wise comparison with the underlying type's `operator==`.
- Stores coordinates inline with no allocation or STL dependency.
