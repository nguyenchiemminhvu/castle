# Fixed-size quaternion rotation primitive

## Overview

A heap-free floating-point quaternion for 3D rotation, composition and rotation-matrix conversion. Components use the `(w, x, y, z)` layout.

## Header

```cpp
#include "castle/math/linalg/quaternion.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `quaternion<T>` | Floating-point quaternion stored entirely in the object. |
| `w()` / `x()` / `y()` / `z()` | Component access. |
| `length()` / `squared_length()` | Quaternion magnitude. |
| `conjugate()` / `inverse()` | Standard quaternion operations. |
| `normalized()` | Produce a unit quaternion suitable for rotation. |
| `operator*` | Hamilton product and rotation composition. |
| `rotate()` | Rotate a 3D vector using the quaternion. |
| `to_matrix()` | Convert to a 3x3 rotation matrix. |
| `from_axis_angle()` | Create a quaternion from a unit-axis rotation in radians. |
| `from_axis_angle_degrees()` | Create a quaternion from an axis and degree angle. |
| `from_euler_xyz()` | Create a quaternion from XYZ Euler angles in radians. |

## Example

```cpp
using castle::math::quaternion;
using castle::math::vector;

const quaternion<float> rotation = quaternion<float>::from_axis_angle_degrees(
    vector<float, 3U>(0.0F, 0.0F, 1.0F),
    90.0F);

const vector<float, 3U> result = rotation.rotate(
    vector<float, 3U>(1.0F, 0.0F, 0.0F));
```

## Embedded notes

- Quaternion storage is four scalar values with no dynamic memory.
- Quaternion elements must be floating-point types.
- Rotation operations normalize the quaternion before using it as a rotation primitive.
- A zero quaternion is rejected by Castle's assertion mechanism for normalization/inversion.
- Multiplication follows the Hamilton convention; `q2 * q1` represents applying `q1` and then `q2` when acting on a vector with `q * p * q.conjugate()`.
- `to_matrix()` produces a conventional 3x3 column-vector rotation matrix compatible with `matrix` and `transform`.
