
# Floating-point near equality

## Overview

Explicit absolute/relative floating-point comparison for callers that need a tolerance tied to their coordinate or measurement units.

## Header

```cpp
#include "castle/math/near_equal.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `near_equal(a, b, epsilon)` | Compare two floating-point values using a scaled tolerance. |

## Example

```cpp
if (castle::math::near_equal(measured, expected, 1e-4f))
{
    // Treat the sensor result as equal within the configured tolerance.
}
```

## Embedded notes

- The comparison uses `epsilon * max(1, abs(a), abs(b))`.
- Pass the tolerance explicitly instead of depending on a global magic constant.
- Typical uses include sensor validation, control tolerances, geometry decisions, and fixed-point-to-floating-point comparisons.
