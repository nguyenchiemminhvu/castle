
# 3D plane

## Overview

Represents an infinite plane by one point and a non-zero normal vector.

## Header

```cpp
#include "castle/math/geometry/plane3d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `plane3d(point, normal)` | Construct a plane. |
| `point()` / `normal()` | Access the plane representation. |
| `signed_value(point)` | Evaluate the signed normal displacement from the plane. |
| `contains(point, epsilon)` | Test whether a point lies on the plane within a tolerance. |

## Example

```cpp
castle::math::plane3d<float> floor(
    castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
    castle::math::vector3d<float>(0.0f, 0.0f, 1.0f));

const bool on_floor =
    floor.contains(sensor_point, 1e-4f);
```

## Embedded notes

- The normal must be non-zero.
- The normal does not have to be unit length; `contains()` accounts for its scale through the supplied absolute tolerance.
- Typical uses include collision planes, clipping, robot frames, and spatial sensor geometry.
