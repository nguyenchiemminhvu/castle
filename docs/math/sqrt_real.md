
# Floating-point square root

## Overview

A deterministic floating-point square-root function intended for geometry and control code that should not depend directly on `<math.h>`.

## Header

```cpp
#include "castle/math/sqrt_real.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `sqrt_real(value)` | Return the floating-point square root for a non-negative input. |

## Example

```cpp
const float distance =
    castle::math::sqrt_real(squared_distance);
```

## Embedded notes

- The implementation uses bounded range reduction and Newton-Raphson iterations.
- The function is `constexpr`.
- Negative inputs are invalid.
- Typical uses include Euclidean distance, vector normalization, RMS calculations, and line/circle geometry.
