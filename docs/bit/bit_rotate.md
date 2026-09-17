# Bit rotation

## Overview

Rotates integer bits left or right. Bits shifted out on one side wrap back to the other side.

## Header

```cpp
#include "castle/bit/bit_rotate.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `rotate_left(value, shift)` | Circular left rotation. |
| `rotate_right(value, shift)` | Circular right rotation. |

## Example

```cpp
uint32_t x = 0x80000001u;
auto y = castle::bit::rotate_left(x, 1);
```

## Embedded notes

- The shift count is reduced modulo the integer width.
- Works on the integer types accepted by Castle bit traits.
