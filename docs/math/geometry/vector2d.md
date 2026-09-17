# Vector2D

## Overview
A small 2D displacement and direction type with common vector operations. Use it for dot products, signed planar cross products, point translation, and normalization when the coordinate type is floating-point.

## Header
`#include "castle/math/geometry/vector2d.hpp"`

## Dependencies
- [`point2d.md`](point2d.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/error_handler.md`](../../core/error_handler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../hypot.md`](../hypot.md)
- [`../square.md`](../square.md)

## Public API
| API | Description |
|---|---|
| `vector2d()` / `vector2d(x, y)` | Construct zero or explicit vectors. |
| `x()`, `y()` | Return components. |
| `squared_length()` | Returns `x*x + y*y`. |
| `length()` | Floating-point Euclidean norm. |
| `dot(other)` | Dot product. |
| `perpendicular()` | Returns `(-y, x)`, a 90-degree counter-clockwise rotation. |
| `cross(other)` | Signed 2D cross product `x1*y2 - y1*x2`. |
| `normalized()` | Floating-point unit-length vector in the same direction. |
| Unary/binary `+`, `-`, `*`, `/`, `+=`, `-=` | Component-wise vector arithmetic and scalar scaling. |
| `operator==`, `operator!=` | Exact component-wise comparison. |
| Free `operator*(scalar, vector)` | Left scalar multiplication. |
| Free `dot(first, second)`, `cross(first, second)` | Convenience wrappers. |
| Free `point - point`, `point + vector`, `point - vector` | Point/vector bridge operations. |

## Usage Example
See `samples/sample_vector2d.cpp`.

```cpp
#include "castle/math/geometry/vector2d.hpp"

castle::math::vector2d<int> v(3, 4);
auto perp = v.perpendicular();
```

## Constraints & Notes
- Accepts arithmetic component types.
- `length()` and `normalized()` are available only for floating-point types.
- `cross()` returns the scalar z component that would result from a 3D cross product of XY-plane vectors.
- Division and normalization assert on zero divisors / zero length.
