# Trigonometry

## Overview
This header wraps the platform C math sine and cosine entry points behind a small Castle interface. Use it from linear algebra, transform, and quaternion code when you want explicit trigonometric dependencies without introducing the C++ standard library.

## Header
`#include "castle/math/linalg/trigonometry.hpp"`

## Dependencies
- [../../core/compiler.hpp](../../core/compiler.md)
- [../../core/traits.hpp](../../core/traits.md)

## Public API
| API | Description |
| --- | --- |
| `sin(float)`, `sin(double)`, `sin(long double)` | Return the sine of a radian angle using `sinf`, `sin`, or `sinl`. |
| `cos(float)`, `cos(double)`, `cos(long double)` | Return the cosine of a radian angle using `cosf`, `cos`, or `cosl`. |
| `template <typename T> radians_sin(T radians)` | Floating-point-only typed sine wrapper. |
| `template <typename T> radians_cos(T radians)` | Floating-point-only typed cosine wrapper. |
| `template <typename T> struct sin_cos_result` | Aggregate containing `.sine` and `.cosine`. |
| `template <typename T> sin_cos(T radians)` | Return both sine and cosine of one radian angle in a single aggregate. |

## Usage Example
```cpp
#include "castle/math/linalg/trigonometry.hpp"
#include "castle/core/constants.hpp"

int main()
{
    const castle::math::sin_cos_result<float> values =
        castle::math::sin_cos(castle::math::pi<float>() * 0.5F);
    (void)values;
}
```
See `samples/sample_trigonometry.cpp`.

## Constraints & Notes
- These functions call the platform C math library directly; Castle does not implement its own sine/cosine approximation in this header.
- Angle units are radians unless a higher-level API explicitly states degrees.
- The wrappers are allocation-free and exception-free, but final precision and determinism depend on the target C math implementation.
- `radians_sin()`, `radians_cos()`, and `sin_cos()` are restricted to floating-point types at compile time.
