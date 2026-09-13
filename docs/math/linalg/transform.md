# Fixed-size 2D and 3D transforms

## Overview

Helpers for building and applying fixed-size affine transforms for 2D and 3D embedded applications. The implementation uses homogeneous matrices and composes transforms without heap allocation.

## Header

```cpp
#include "castle/math/linalg/transform.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `translation_2d()` / `translation_3d()` | Construct homogeneous translations. |
| `scaling_2d()` / `scaling_3d()` | Construct homogeneous non-uniform scales. |
| `rotation_2d()` | Build a 2D rotation from radians. |
| `rotation_2d_degrees()` | Build a 2D rotation from degrees. |
| `rotation_x()` / `rotation_y()` / `rotation_z()` | Build 3D principal-axis rotations in radians. |
| Degree rotation overloads | Principal-axis rotations specified in degrees. |
| `rotation_axis_angle_3d()` / `rotation_axis_angle()` | Build axis-angle rotations in 3x3 or homogeneous 4x4 form. |
| `rotation_axis_angle_degrees()` | Axis-angle rotation specified in degrees. |
| `rotation_xyz()` | Compose X, Y and Z rotations. |
| `compose_transform_2d()` / `compose_transform_3d()` | Compose scale, rotation and translation into one transform. |
| `transform_point_2d()` / `transform_point_3d()` | Apply translation-aware point transforms. |
| `transform_vector_2d()` / `transform_vector_3d()` | Apply direction transforms without translation. |
| Geometry overloads | Interoperate with Castle `point2d`, `point3d`, `vector2d` and `vector3d`. |

## Example

```cpp
const castle::math::matrix<float, 4U, 4U> transform =
    castle::math::translation_3d(10.0F, 20.0F, 30.0F) *
    castle::math::rotation_z_degrees(90.0F) *
    castle::math::scaling_3d(2.0F, 2.0F, 2.0F);

const castle::math::vector<float, 3U> result =
    castle::math::transform_point_3d(
        transform,
        castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));
```

## Embedded notes

- Matrices are row-major, but transforms use the conventional column-vector model: `result = matrix * vector`.
- Translation is stored in the final matrix column.
- `T * R * S` therefore applies scale first, then rotation, then translation.
- Point transforms use homogeneous coordinate `w = 1`; vector transforms use `w = 0`, so translations do not affect directions.
- Axis-angle construction requires a non-zero axis; normalization errors are reported through Castle assertions.
- When a transform produces a homogeneous coordinate other than one, point helpers perform the required perspective divide.
