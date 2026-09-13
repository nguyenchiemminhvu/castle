
# Infinite 3D line

## Overview

Represents an infinite three-dimensional line as an origin point plus a direction vector.

## Header

```cpp
#include "castle/math/geometry/line3d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `line3d(origin, direction)` | Construct a parameterized line. |
| `line3d(first, second)` | Construct a line through two points. |
| `origin()` / `direction()` | Access the line representation. |
| `point_at(parameter)` | Evaluate `origin + direction * parameter`. |
| `degenerate()` | Detect a zero direction. |

## Example

```cpp
castle::math::line3d<float> ray(
    castle::math::point3d<float>(0.0f, 0.0f, 1.0f),
    castle::math::vector3d<float>(0.0f, 0.0f, -1.0f));
```

## Embedded notes

- The type itself represents an infinite line. Applications that need a ray or segment should enforce parameter limits explicitly.
- Typical uses include sensor rays, robot trajectories, and analytic intersection calculations.
