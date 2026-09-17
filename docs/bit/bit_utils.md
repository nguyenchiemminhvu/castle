# Bit field and set-bit helpers

## Overview

Higher-level helpers for extracting individual set bits and reading bit fields from packed values.

## Header

```cpp
#include "castle/bit/bit_utils.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `extract_lowest_set_bit()` | Keep only the lowest set bit. |
| `extract_highest_set_bit()` | Keep only the highest set bit; zero stays zero. |
| `extract_field()` | Read a contiguous field and shift it down to bit zero. |
| Related field helpers | The header contains complementary helpers for working with packed bit fields. |

## Example

```cpp
uint32_t reg = 0x0000A500u;
auto field = castle::bit::extract_field(reg, 8, 8);
```

## Embedded notes

- Field positions are counted from the least-significant bit.
- Runtime field operations guard zero width and out-of-range starts.
