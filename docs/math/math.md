# Math

## Overview
Umbrella header that pulls in Castle's scalar math, ratio, random, geometry, and linear-algebra facilities, plus the lightweight `is_equal()` and `is_zero()` helpers.

## Header
`#include "castle/math/math.hpp"`

## Dependencies
- Core: [compiler](../core/compiler.md), [traits](../core/traits.md), [type_ranges](../core/type_ranges.md), [constants](../core/constants.md)
- Math: [fib](fib.md), [gcd](gcd.md), [lcm](lcm.md), [abs](abs.md), [sqrt](sqrt.md), [clamp](clamp.md), [mean](mean.md), [ratio](ratio.md), [invert](invert.md), [logarithm](logarithm.md), [random](random.md), [ceil_div](ceil_div.md), [floor_div](floor_div.md), [hypot](hypot.md), [factorial](factorial.md), [is_power_of_two](is_power_of_two.md), [isqrt](isqrt.md), [lerp](lerp.md), [mod](mod.md), [near_equal](near_equal.md), [powi](powi.md), [saturating](saturating.md), [sign](sign.md), [sqrt_real](sqrt_real.md), [square](square.md), [geometry](geometry.md), [linalg/vector](linalg/vector.md), [linalg/matrix](linalg/matrix.md), [linalg/quaternion](linalg/quaternion.md), [linalg/transform](linalg/transform.md), [linalg/trigonometry](linalg/trigonometry.md)

## Public API
| API | Description |
|---|---|
| `#include "castle/math/math.hpp"` | Imports the Castle math umbrella, including scalar math helpers, compile-time utilities, PRNG support, geometry, and linear algebra. |
| `template <typename T> bool castle::math::is_equal(T a, T b)` | Floating-point absolute-epsilon comparison using `meta::floating_epsilon<T>::value`. |
| `template <typename T> bool castle::math::is_zero(T a)` | Floating-point zero check using `meta::floating_epsilon<T>::value`. |

## Usage Example
See `samples/sample_math.cpp` for a complete example.

```cpp
static_assert(castle::sqrt<81U>::value == 9U, "");
const bool equal = castle::math::is_equal(1.0f, 1.0f);
const int clamped = castle::math::clamp(150, 0, 100);
```

## Constraints & Notes
- This umbrella increases compile-time dependencies; prefer narrower headers when footprint matters.
- `is_equal()` and `is_zero()` use absolute machine epsilon only; use `near_equal()` for caller-controlled tolerances.
- Behavior remains deterministic and allocation-free.
