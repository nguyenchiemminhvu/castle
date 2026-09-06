# Bit masks

## Overview

Builds masks for individual bits and ranges of low or high bits.

## Header

```cpp
#include "castle/bit/bit_mask.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `all_bits_mask<T>::value` | Compile-time mask with every bit set. |
| `single_bit_mask<T>(index)` | Create a mask with one selected bit set. |
| `single_bit_mask_const<index, T>::value` | Compile-time single-bit mask. |
| `low_bits_mask<T>(count)` | Mask containing the lowest `count` bits. |
| High/range mask helpers | The header also provides related masks for the upper or selected bit ranges. |

## Example

```cpp
auto bit3 = castle::bit::single_bit_mask<uint8_t>(3);
auto low4 = castle::bit::low_bits_mask<uint8_t>(4);
```

## Embedded notes

- Runtime mask builders handle full-width counts without shifting by the type width.
- Compile-time forms reject invalid indexes with `static_assert`.
