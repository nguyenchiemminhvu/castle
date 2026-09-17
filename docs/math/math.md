# Math umbrella header

## Overview

Convenience include for the original Castle math utilities.

Feature-specific additions such as integer division, saturation, interpolation, floating-point square root, and geometry remain available through their own headers so callers can keep dependency graphs small.

## Header

```cpp
#include "castle/math/math.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Math helpers | Includes the existing scalar math, ratio, random, root, mean, GCD/LCM and related headers. |

## Additional feature-specific headers

| Header | Main API |
|---|---|
| `angle.h` | `pi()`, `tau()`, angle conversion. |
| `ceil_div.h` / `floor_div.h` | Mathematical integer division. |
| `hypot.h` / `sqrt_real.h` | Floating-point Euclidean norm and square root. |
| `is_power_of_two.h` / `isqrt.h` | Discrete integer math helpers. |
| `lerp.h` / `powi.h` | Interpolation and integer-exponent arithmetic. |
| `minmax.h` / `sign.h` / `square.h` | Small scalar building blocks. |
| `mod.h` / `near_equal.h` | Wrapping and floating-point tolerance helpers. |
| `saturating.h` | Overflow-safe integer add/subtract. |
| `geometry.h` | 2D/3D geometry primitives, algorithms, and intersections. |

## Example

```cpp
auto x = castle::math::clamp(120, 0, 100);
const auto pages = castle::math::ceil_div(1025U, 256U);
```

## Embedded notes

- Prefer feature-specific headers for smaller dependency graphs.
- The new math helpers favor `constexpr`, deterministic arithmetic, and no dynamic allocation.
- Geometry has its own umbrella header so applications that do not use geometry do not need to include it.
