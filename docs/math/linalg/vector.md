# Vector

## Overview
`castle::math::vector<T, N>` is a fixed-size mathematical vector with inline storage for deterministic embedded numeric work. Use it when the dimension is known at compile time and you need component-wise arithmetic, dot and cross products, normalization, and related helpers without dynamic allocation or STL containers.

## Header
`#include "castle/math/linalg/vector.hpp"`

## Dependencies
- [../../core/compiler.hpp](../../core/compiler.md)
- [../../core/error_handler.hpp](../../core/error_handler.md)
- [../../core/traits.hpp](../../core/traits.md)
- [../../core/types.hpp](../../core/types.md)
- [../abs.hpp](../abs.md)
- [../lerp.hpp](../lerp.md)
- [../near_equal.hpp](../near_equal.md)
- [../sqrt_real.hpp](../sqrt_real.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T, size_type N> class vector` | Fixed-size vector of `N` arithmetic values stored contiguously inside the object. |
| `vector()` / `explicit vector(Args&&... args)` | Zero-initialize or construct from exactly `N` component values in index order. |
| `size()`, `begin()`, `end()`, `data()` | Query the dimension and access contiguous inline storage. |
| `operator[]`, `at()` | Unchecked and checked component access; `at()` asserts on out-of-range indices. |
| Unary/binary arithmetic operators | Component-wise add, subtract, negate, and scale by a scalar. |
| `squared_length()`, `length()` | Return the squared Euclidean norm or the floating-point Euclidean norm. |
| `dot()`, `hadamard()` | Return the dot product or component-wise product with another vector. |
| `component_min()`, `component_max()`, `abs()` | Return component-wise min, max, or absolute value vectors. |
| `normalized()` | Return a unit-length copy; asserts on the zero vector. |
| `distance_to()`, `lerp_to()` | Floating-point distance and per-component linear interpolation. |
| `operator*(scalar, vector)` | Scalar-left scaling helper. |
| `vector2<T>`, `vector3<T>`, `vector4<T>` | Convenience aliases for common dimensions. |
| `dot(first, second)`, `hadamard(first, second)` | Free-function wrappers around the corresponding member operations. |
| `project(value, onto)` | Project `value` onto `onto`; asserts when `onto` is the zero vector. |
| `reflect(value, normal)` | Reflect using `value - normal * (2 * value.dot(normal))`; the normal is not normalized automatically. |
| `cross(first, second)` | 3D right-hand-rule cross product. |
| `scalar_triple_product(first, second, third)` | Signed 3D volume scale factor `dot(first, cross(second, third))`. |
| `near_equal(first, second, epsilon)` | Relative-tolerance comparison for floating-point vectors. |

## Usage Example
```cpp
#include "castle/math/linalg/vector.hpp"

using castle::math::vector;

int main()
{
    const vector<float, 3U> a(1.0F, 2.0F, 3.0F);
    const vector<float, 3U> b(4.0F, 5.0F, 6.0F);
    const float projection = a.dot(b);
    const vector<float, 3U> unit = a.normalized();
    const vector<float, 3U> normal(0.0F, 0.0F, 1.0F);
    const vector<float, 3U> reflected = castle::math::reflect(a, normal);
    (void)projection;
    (void)unit;
    (void)reflected;
}
```
See `samples/sample_linalg_vector.cpp`.

## Constraints & Notes
- Storage is contiguous and embedded directly in the object; there is no heap allocation.
- `length()`, `normalized()`, `distance_to()`, `lerp_to()`, and `near_equal()` are available only for floating-point element types.
- `length()` and `normalized()` use `castle::math::sqrt_real`, not `std::sqrt`.
- `project()` preserves the arithmetic semantics of `T`; for integral `T`, the division is integral.
- `reflect()` assumes the supplied normal already has the magnitude appropriate for the caller's reflection formula.
