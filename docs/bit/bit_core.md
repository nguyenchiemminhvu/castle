# Basic bit operations

## Overview

Small, constexpr-friendly helpers for testing and changing individual bits in integer values.

## Header

```cpp
#include "castle/bit/bit_core.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `test(value, bit_index)` | Check one bit. Out-of-range runtime indexes return `false`. |
| `set(value, bit_index)` | Set one bit. Out-of-range runtime indexes leave the value unchanged. |
| `clear(value, bit_index)` | Clear one bit. |
| Compile-time overloads | Several operations also accept the bit index as a template argument and check the range with `static_assert`. |

## Example

```cpp
uint8_t v = 0;
v = castle::bit::set(v, 3);
bool on = castle::bit::test(v, 3);
v = castle::bit::clear(v, 3);
```

## Embedded notes

- Integer types are validated through Castle type traits.
- Bit indexes are zero-based.
