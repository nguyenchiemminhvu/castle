
# Integer floor division

## Overview

Computes mathematical floor division for integral values with a positive denominator. This differs from C++ integer division, which truncates toward zero.

## Header

```cpp
#include "castle/math/floor_div.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `floor_div(numerator, denominator)` | Return `floor(numerator / denominator)` for integral values. |

## Example

```cpp
const int32_t cell =
    castle::math::floor_div(world_coordinate, cell_size);
```

## Embedded notes

- The denominator must be positive.
- This is especially useful for signed coordinates where negative values must map to the mathematically correct lower grid cell.
- Typical uses include map-cell lookup, fixed-point quantization, and signed raster coordinates.
