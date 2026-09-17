
# Geometry umbrella header

## Overview

Convenience include for all public Castle geometry primitives, algorithms, and intersection helpers.

## Header

```cpp
#include "castle/math/geometry/geometry.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `point2d` / `point3d` | Two-dimensional and three-dimensional positions. |
| `vector2d` / `vector3d` | Direction, displacement, and vector products. |
| `line2d` / `line3d` | Infinite parameterized lines. |
| `plane3d` | Infinite 3D plane. |
| `circle2d` / `circle3d` | Circular regions and circumferences. |
| `polygon2d` | Fixed-capacity polygon stored in `castle::container::array`. |
| Algorithms and intersections | Nearest-point, hull, containment, and analytic intersection operations. |

## Example

```cpp
#include "castle/math/geometry/geometry.h"

castle::math::vector3d<float> normal(0.0f, 0.0f, 1.0f);
castle::math::plane3d<float> ground(
    castle::math::point3d<float>(0.0f, 0.0f, 0.0f), normal);
```

## Embedded notes

- Use feature-specific headers when a smaller dependency graph is preferred.
- Geometry algorithms use fixed storage and do not allocate from the heap.
