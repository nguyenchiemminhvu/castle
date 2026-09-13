
# Integer square root

## Overview

Returns the floor of the square root for integral values without requiring floating-point arithmetic.

## Header

```cpp
#include "castle/math/isqrt.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `isqrt(value)` | Return the largest integer `r` such that `r*r <= value`. |

## Example

```cpp
const uint32_t distance =
    castle::math::isqrt(squared_distance);
```

## Embedded notes

- Unsigned inputs are handled directly; signed inputs must be non-negative.
- The comparison uses division instead of `middle * middle`, avoiding multiplication overflow in the search.
- The helper is `constexpr`.
- Typical uses include fixed-point geometry, integer Euclidean distance, grid sizing, and lookup-table generation.
