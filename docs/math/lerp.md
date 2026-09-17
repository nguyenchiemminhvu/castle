
# Linear interpolation

## Overview

A small floating-point linear interpolation helper. The interpolation parameter is not clamped.

## Header

```cpp
#include "castle/math/lerp.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `lerp(a, b, t)` | Return `a + t * (b - a)`. |

## Example

```cpp
const float filtered =
    castle::math::lerp(previous, target, 0.1f);
```

## Embedded notes

- Floating-point types are required so fractional interpolation is not silently discarded.
- Values of `t` outside `[0, 1]` intentionally perform extrapolation.
- Typical uses include sensor calibration, ramp generation, actuator set-point changes, and lookup-table interpolation.
