# Vector3D

## Overview
A small 3D displacement and direction type with dot, cross, and scalar triple products. Use it for spatial vector math, point translation, and normalization when the coordinate type is floating-point.

## Header
`#include "castle/math/geometry/vector3d.hpp"`

## Dependencies
- [`point3d.md`](point3d.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/error_handler.md`](../../core/error_handler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../hypot.md`](../hypot.md)

## Public API
| API | Description |
|---|---|
| `vector3d()` / `vector3d(x, y, z)` | Construct zero or explicit vectors. |
| `x()`, `y()`, `z()` | Return components. |
| `squared_length()` | Returns `x*x + y*y + z*z`. |
| `length()` | Floating-point Euclidean norm. |
| `degenerate()` | Returns true when all components are exactly zero. |
| `dot(other)` | Dot product. |
| `cross(other)` | Right-handed 3D cross product. |
| `normalized()` | Floating-point unit-length vector in the same direction. |
| Unary/binary `+`, `-`, `*`, `/`, `+=`, `-=` | Component-wise vector arithmetic and scalar scaling. |
| `operator==`, `operator!=` | Exact component-wise comparison. |
| Free `operator*(scalar, vector)` | Left scalar multiplication. |
| Free `point - point`, `point + vector`, `point - vector` | Point/vector bridge operations. |
| Free `dot(a, b)`, `cross(a, b)` | Convenience wrappers. |
| `scalar_triple_product(a, b, c)` | Returns `a dot (b cross c)`. |

## Usage Example
See `samples/sample_vector3d.cpp`.

```cpp
#include "castle/math/geometry/vector3d.hpp"

castle::math::vector3d<int> x_axis(1, 0, 0);
castle::math::vector3d<int> y_axis(0, 1, 0);
auto z_axis = x_axis.cross(y_axis);
```

## Constraints & Notes
- Accepts arithmetic component types.
- `length()` and `normalized()` are available only for floating-point types.
- `cross()` and `scalar_triple_product()` follow right-handed orientation.
- Division and normalization assert on zero divisors / zero length.
