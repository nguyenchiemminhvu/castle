
# Angle constants and conversions

## Overview

Small floating-point angle helpers for radians/degrees conversion and compile-time-friendly mathematical constants.

## Header

```cpp
#include "castle/math/angle.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `pi<T>()` | Return π in the caller's floating-point type. |
| `tau<T>()` | Return 2π in the caller's floating-point type. |
| `degrees_to_radians()` | Convert an angle from degrees to radians. |
| `radians_to_degrees()` | Convert an angle from radians to degrees. |

## Example

```cpp
const float heading_deg = 90.0f;
const float heading_rad = castle::math::degrees_to_radians(heading_deg);
```

## Embedded notes

- The helpers are `constexpr`, so fixed calibration angles can be evaluated at compile time.
- Use radians internally when passing values to trigonometric or control-law code.
- Typical uses include sensor orientation, motor commutation, attitude control, and coordinate transforms.
