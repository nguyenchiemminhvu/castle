
# Integer ceiling division

## Overview

Computes mathematical ceiling division for integral values with a positive denominator without using the common `a + b - 1` expression.

## Header

```cpp
#include "castle/math/ceil_div.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `ceil_div(numerator, denominator)` | Return `ceil(numerator / denominator)` for integral values. |

## Example

```cpp
const castle::size_type dma_pages =
    castle::math::ceil_div(buffer_bytes, page_bytes);
```

## Embedded notes

- The denominator must be positive.
- The implementation avoids the intermediate addition used by `(a + b - 1) / b`, which can overflow near the maximum value.
- Typical uses include DMA transfer counts, flash-page counts, packet counts, and block allocation.
