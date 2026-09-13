
# 3D vector

## Overview

Represents a three-dimensional direction or displacement with dot, cross, and scalar-triple products.

## Header

```cpp
#include "castle/math/geometry/vector3d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `vector3d<T>` | Store `x_`, `y_`, and `z_` vector components. |
| `squared_length()` / `length()` | Return squared or Euclidean magnitude. |
| `degenerate()` | Test whether the vector is zero. |
| `dot()` | Return the scalar dot product. |
| `cross()` | Return the vector cross product. |
| `normalized()` | Return a unit vector for floating-point vectors. |
| `scalar_triple_product()` | Return `a · (b × c)`. |

## Example

```cpp
const castle::math::vector3d<float> x_axis(1.0f, 0.0f, 0.0f);
const castle::math::vector3d<float> y_axis(0.0f, 1.0f, 0.0f);

const auto z_axis = castle::math::cross(x_axis, y_axis);
```

## Embedded notes

- Use `cross()` for normals and orientation.
- Use the scalar triple product for signed volume and coplanarity tests.
- Typical uses include 3D robotics, graphics-free spatial collision logic, sensor frames, and mechanical geometry.
