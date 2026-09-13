
# Positive modulo and wrapping

## Overview

Integer helpers for converting signed modulo results into a positive range and wrapping values into a half-open interval.

## Header

```cpp
#include "castle/math/mod.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `positive_mod(value, modulus)` | Return a result in `[0, modulus)`. |
| `wrap(value, low, high)` | Return a value in `[low, high)` by periodic wrapping. |

## Example

```cpp
const int phase =
    castle::math::wrap(raw_phase, 0, phase_period);
```

## Embedded notes

- The modulus must be positive and `high` must be greater than `low`.
- `wrap()` widens common 8/16/32-bit intermediate calculations before subtraction.
- Typical uses include encoder counters, timer phases, circular buffers, and periodic integer state machines.
