# Linear Algebra Aggregator

## Overview
This header aggregates Castle's individual fixed-size linear algebra components inside the `math/linalg` subtree. Include it when you want vectors, matrices, trigonometric wrappers, quaternions, and transforms together without going through the higher-level umbrella path.

## Header
`#include "castle/math/linalg/linalg.hpp"`

## Dependencies
- [vector.hpp](vector.md)
- [matrix.hpp](matrix.md)
- [trigonometry.hpp](trigonometry.md)
- [quaternion.hpp](quaternion.md)
- [transform.hpp](transform.md)

## Public API
| API | Description |
| --- | --- |
| `#include "castle/math/linalg/linalg.hpp"` | Includes `vector.hpp`, `matrix.hpp`, `trigonometry.hpp`, `quaternion.hpp`, and `transform.hpp`. |
| `castle::math::vector<T, N>` | Fixed-size inline vectors. |
| `castle::math::matrix<T, Rows, Columns>` | Fixed-size row-major matrices for column-vector math. |
| `castle::math::sin_cos(...)` and related trig wrappers | Thin floating-point wrappers around the platform C math sine/cosine functions. |
| `castle::math::quaternion<T>` | Fixed-size `(w, x, y, z)` quaternion rotations. |
| Transform builders and application helpers | 2D and 3D homogeneous translation, rotation, scale, composition, and point/vector transforms. |

## Usage Example
```cpp
#include "castle/math/linalg/linalg.hpp"

int main()
{
    const castle::math::quaternion<float> rotation =
        castle::math::quaternion<float>::from_euler_xyz(0.0F, 0.0F, 1.57079632679F);
    const castle::math::vector<float, 3U> result =
        rotation.rotate(castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));
    (void)result;
}
```
See `samples/sample_linalg_all.cpp`.

## Constraints & Notes
- This header adds no new runtime logic; it only aggregates the individual linear algebra components.
- All aggregated types are fixed-size and keep storage inline with no heap allocation.
- Matrices are row-major, vectors are treated as columns, and quaternion/transform rotation helpers rely on floating-point sine/cosine from the platform C math library.
