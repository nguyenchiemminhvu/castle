
# Integer-power exponentiation

## Overview

Computes a value raised to a non-negative integer exponent using exponentiation by squaring.

## Header

```cpp
#include "castle/math/powi.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `powi(base, exponent)` | Return `base^exponent` for an unsigned integer exponent. |

## Example

```cpp
const float squared_error = castle::math::powi(error, 2U);
```

## Embedded notes

- Runtime multiplication count is `O(log(exponent))`.
- The function is `constexpr`.
- Arithmetic still follows the selected type's overflow/precision rules.
- Typical uses include calibration polynomials, fixed-point gain scaling, and compile-time lookup-constant generation.
