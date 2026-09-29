# Trigonometry

## Overview
This header wraps the platform C math sine, cosine, arc tangent, and arc sine entry points behind a small Castle interface. Use it from linear algebra, transform, and quaternion code when you want explicit trigonometric dependencies without introducing the C++ standard library.

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
| `atan2(float, float)`, `atan2(double, double)`, `atan2(long double, long double)` | Return the arc tangent of `y / x` using `atan2f`, `atan2`, or `atan2l`, determining the quadrant from both argument signs. |
| `asin(float)`, `asin(double)`, `asin(long double)` | Return the arc sine of a value in `[-1, 1]` using `asinf`, `asin`, or `asinl`. |
| `template <typename T> radians_sin(T radians)` | Floating-point-only typed sine wrapper. |
| `template <typename T> radians_cos(T radians)` | Floating-point-only typed cosine wrapper. |
| `template <typename T> radians_atan2(T y, T x)` | Floating-point-only typed arc tangent wrapper. |
| `template <typename T> radians_asin(T value)` | Floating-point-only typed arc sine wrapper. |
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
- `radians_sin()`, `radians_cos()`, `radians_atan2()`, `radians_asin()`, and `sin_cos()` are restricted to floating-point types at compile time.
- `asin()` and `radians_asin()` expect the input to lie in `[-1, 1]`; behavior for out-of-range inputs follows the platform C math library.
