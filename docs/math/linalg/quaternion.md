# Quaternion

## Overview
`castle::math::quaternion<T>` stores a 3D rotation in fixed-size `(w, x, y, z)` form. Use it when you need compact rotation storage, Hamilton-product composition, vector rotation, or conversion to a 3x3 rotation matrix without heap allocation or STL dependencies.

## Header
`#include "castle/math/linalg/quaternion.hpp"`

## Dependencies
- [../../core/compiler.hpp](../../core/compiler.md)
- [../../core/error_handler.hpp](../../core/error_handler.md)
- [../../core/traits.hpp](../../core/traits.md)
- [../../core/constants.hpp](../../core/constants.md)
- [../sqrt_real.hpp](../sqrt_real.md)
- [matrix.hpp](matrix.md)
- [trigonometry.hpp](trigonometry.md)
- [vector.hpp](vector.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T> class quaternion` | Floating-point quaternion stored as `(w, x, y, z)`. |
| `quaternion()` / `quaternion(T w, T x, T y, T z)` | Construct the identity rotation or explicit coefficients. |
| `w()`, `x()`, `y()`, `z()` | Return the stored quaternion components. |
| `squared_length()`, `length()` | Return the squared norm or norm. |
| `conjugate()`, `normalized()`, `inverse()` | Return the conjugate, unit quaternion, or multiplicative inverse; normalization/inversion assert on the zero quaternion. |
| `operator*(other)` | Hamilton product; when used as active rotations, `lhs * rhs` applies `rhs` first and then `lhs`. |
| `operator/(scalar)` | Divide all stored coefficients by a scalar; asserts on zero. |
| `rotate(value)` | Rotate a 3D column vector using `q * p * q.conjugate()` after normalizing `q`. |
| `to_matrix()` | Convert to a row-major 3x3 rotation matrix that acts on column vectors. |
| `operator==`, `operator!=` | Exact component-wise comparisons. |
| `from_axis_angle(axis, radians)` | Build a unit quaternion from a normalized axis and radian angle; normalizes the axis internally. |
| `from_axis_angle_degrees(axis, degrees)` | Degree-based axis-angle constructor. |
| `from_euler_xyz(x, y, z)` | Build the quaternion equivalent of applying X, then Y, then Z rotations to a column vector. |

## Usage Example
```cpp
#include "castle/math/linalg/quaternion.hpp"

int main()
{
    const castle::math::quaternion<float> rotation =
        castle::math::quaternion<float>::from_axis_angle_degrees(
            castle::math::vector<float, 3U>(0.0F, 0.0F, 1.0F),
            90.0F);
    const castle::math::vector<float, 3U> result =
        rotation.rotate(castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));
    (void)result;
}
```
See `samples/sample_quaternion.cpp`.

## Constraints & Notes
- The element type must be floating point.
- Rotation-related helpers normalize the quaternion before use, so scaled quaternions represent the same orientation.
- `to_matrix()` returns a row-major matrix that multiplies column vectors, matching the rest of Castle linear algebra.
- `from_axis_angle()` and `from_axis_angle_degrees()` assert when the axis is the zero vector because normalization is required.
