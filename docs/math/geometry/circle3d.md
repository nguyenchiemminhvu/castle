
# 3D circle

## Overview

Represents a planar 3D circle by center, plane normal, and non-negative radius.

## Header

```cpp
#include "castle/math/geometry/circle3d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `circle3d(center, normal, radius)` | Construct a planar circle. |
| `center()` / `normal()` / `radius()` | Access circle parameters. |
| `contains(point, epsilon)` | Test membership in the filled planar disk. |
| `on_circle(point, epsilon)` | Test membership on the circumference. |
| `plane()` | Return the circle's supporting plane. |

## Example

```cpp
castle::math::circle3d<float> target(
    castle::math::point3d<float>(0.0f, 0.0f, 1.0f),
    castle::math::vector3d<float>(0.0f, 0.0f, 1.0f),
    0.5f);
```

## Embedded notes

- The normal must be non-zero and the radius non-negative.
- The normal does not have to be normalized.
- `contains()` is a planar disk test, not a spherical volume test.
- Typical uses include planar sensor targets, circular mechanical paths, collision rings, and spatial intersection calculations.
