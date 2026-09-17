
# Scalar min and max

## Overview

Deterministic scalar minimum and maximum helpers without an STL dependency.

## Header

```cpp
#include "castle/math/minmax.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `min(a, b)` | Return the smaller value. |
| `max(a, b)` | Return the larger value. |
| `min3(a, b, c)` | Return the smallest of three values. |
| `max3(a, b, c)` | Return the largest of three values. |

## Example

```cpp
const int command = castle::math::max(
    minimum_command,
    castle::math::min(requested_command, maximum_command));
```

## Embedded notes

- All helpers are `constexpr`.
- Typical uses include actuator limits, sensor bounds, thresholds, and small arithmetic kernels where bringing in `<algorithm>` is undesirable.
