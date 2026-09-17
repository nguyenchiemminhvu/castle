
# 3D point

## Overview

Represents a position in a three-dimensional coordinate system.

## Header

```cpp
#include "castle/math/geometry/point3d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `point3d<T>` | Store `x_`, `y_`, and `z_` coordinates. |
| `x()` / `y()` / `z()` | Read coordinates. |
| `set_x()` / `set_y()` / `set_z()` | Modify coordinates. |
| `operator==` / `operator!=` | Compare coordinates exactly. |

## Example

```cpp
castle::math::point3d<float> sensor_position(1.0f, 2.0f, 0.5f);
```

## Embedded notes

- `T` must be an arithmetic type.
- Typical uses include robot/world coordinates, 3D sensor measurements, CAD vertices, and spatial waypoints.
