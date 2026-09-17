
# Euclidean norm

## Overview

Overflow-resistant two-dimensional and three-dimensional Euclidean norm helpers for floating-point values.

## Header

```cpp
#include "castle/math/hypot.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `hypot(x, y)` | Return `sqrt(x*x + y*y)` using scale normalization. |
| `hypot(x, y, z)` | Return `sqrt(x*x + y*y + z*z)` using scale normalization. |

## Example

```cpp
const float radial_error =
    castle::math::hypot(error_x, error_y);
```

## Embedded notes

- The largest magnitude is factored out before squaring, reducing overflow risk compared with a direct `sqrt_real(x*x + y*y)`.
- The functions are `constexpr`.
- Typical uses include distance calculations, vector norms, RMS-style calculations, and control-error magnitude.
