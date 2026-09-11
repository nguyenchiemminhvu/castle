# Bit and byte reversal

## Overview

Reverses bit order and byte order for fixed-width integer values. These helpers are useful for protocol and register work.

## Header

```cpp
#include "castle/bit/bit_reverse.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `reverse_bits()` | Reverse all bits in an 8-, 16-, 32-, or 64-bit value. |
| `byte_swap()` | Reverse byte order for the supported integer widths. |

## Example

```cpp
uint8_t x = 0b10110000;
auto r = castle::bit::reverse_bits(x); // 0b00001101
```

## Embedded notes

- `reverse_bits()` changes bit order inside the full width of the type.
- `byte_swap()` is about endianness; it does not reverse individual bits.
