# Transform

## Overview
This header builds and applies fixed-size 2D and 3D homogeneous transforms. Use it when you need deterministic translation, rotation, scale, and composition helpers that operate on Castle vectors, points, and geometry types without dynamic allocation or STL containers.

## Header
`#include "castle/math/linalg/transform.hpp"`

## Dependencies
- [../../core/compiler.hpp](../../core/compiler.md)
- [../../core/error_handler.hpp](../../core/error_handler.md)
- [../../core/constants.hpp](../../core/constants.md)
- [../geometry/point2d.hpp](../geometry/point2d.md)
- [../geometry/point3d.hpp](../geometry/point3d.md)
- [../geometry/vector2d.hpp](../geometry/vector2d.md)
- [../geometry/vector3d.hpp](../geometry/vector3d.md)
- [matrix.hpp](matrix.md)
- [trigonometry.hpp](trigonometry.md)
- [vector.hpp](vector.md)

## Public API
| API | Description |
| --- | --- |
| `translation_2d(...)`, `translation_3d(...)` | Build 3x3 or 4x4 row-major translation matrices with translation stored in the final column. |
| `scaling_2d(...)`, `scaling_3d(...)` | Build 3x3 or 4x4 row-major non-uniform scaling matrices. |
| `rotation_2d(...)`, `rotation_2d_degrees(...)` | Build 2D homogeneous rotation matrices from radians or degrees. |
| `rotation_x/y/z(...)`, `rotation_x/y/z_degrees(...)` | Build principal-axis 3D rotation matrices from radians or degrees. |
| `rotation_axis_angle_3d(axis, radians)` | Build a 3x3 axis-angle rotation matrix after normalizing the axis. |
| `rotation_axis_angle(axis, radians)` / `rotation_axis_angle_degrees(axis, degrees)` | Build a homogeneous 4x4 axis-angle rotation matrix. |
| `rotation_xyz(x, y, z)` | Return `rotation_z(z) * rotation_y(y) * rotation_x(x)`, which applies X, then Y, then Z rotation to a column vector. |
| `compose_transform_2d(translation, rotation, scale)` | Return `translation_2d(...) * rotation_2d(...) * scaling_2d(...)`, so scale happens first, then rotation, then translation. |
| `compose_transform_3d(translation, x, y, z, scale)` | Return `translation_3d(...) * rotation_xyz(...) * scaling_3d(...)`. |
| `transform_point_2d/3d(...)` | Apply a homogeneous transform with `w = 1`; performs the homogeneous divide if the result is not already normalized. |
| `transform_vector_2d/3d(...)` | Apply a homogeneous transform with `w = 0`, so translation does not affect the result. |
| Geometry overloads | `point2d`, `point3d`, `vector2d`, and `vector3d` adapters that delegate through `vector<T, N>`. |

## Usage Example
```cpp
#include "castle/math/linalg/transform.hpp"
#include "castle/core/constants.hpp"

int main()
{
    const castle::math::matrix<float, 3U, 3U> transform =
        castle::math::compose_transform_2d(
            castle::math::vector<float, 2U>(2.0F, 3.0F),
            castle::math::degrees_to_radians(90.0F),
            castle::math::vector<float, 2U>(2.0F, 1.0F));
    const castle::math::vector<float, 2U> result =
        castle::math::transform_point_2d(
            transform,
            castle::math::vector<float, 2U>(1.0F, 0.0F));
    (void)result;
}
```
See `samples/sample_transform.cpp`.

## Constraints & Notes
- Storage is row-major, but transforms multiply column vectors: `result = matrix * vector`.
- Because translation is stored in the final column, `A * B` applies `B` first and then `A`.
- `compose_transform_2d()` and `compose_transform_3d()` therefore apply scale first, then rotation, then translation.
- Axis-angle helpers normalize the input axis and assert when it is the zero vector.
- Point-transform helpers assert when the resulting homogeneous coordinate is zero.
