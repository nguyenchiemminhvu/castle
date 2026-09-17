
# Geometry entry point

## Overview

Entry-point header for Castle's fixed-capacity two-dimensional and three-dimensional geometry types and algorithms.

## Header

```cpp
#include "castle/math/geometry.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Geometry primitives | Points, vectors, lines, planes, circles, and fixed-capacity polygons. |
| Geometry algorithms | Distance, nearest-point, point-in-polygon, and convex-hull operations. |
| Intersection algorithms | Line, plane, and circle intersections. |

## Example

```cpp
castle::math::point2d<float> robot_position(1.0f, 2.0f);
castle::math::circle2d<float> safety_zone(robot_position, 5.0f);

const bool inside = safety_zone.contains(
    castle::math::point2d<float>(3.0f, 2.0f));
```

## Embedded notes

- The geometry layer is header-only and uses Castle containers for fixed-capacity storage.
- Integer point/vector/polygon types are suitable for deterministic grid geometry.
- Continuous calculations such as intersections, normalization, and Euclidean distance use floating-point types.
