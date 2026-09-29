# CRC-8

## Overview
Implements CRC-8/SMBUS with polynomial `0x07`, initial value `0x00`, no final XOR, and non-reflected input/output.

## Header
`#include "castle_ext/checksums/crc8.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::crc8` | Stateful CRC-8 calculator. |
| `reset()` | Restores the initial CRC state. |
| `update(data, size)` | Adds bytes to the running CRC; O(size). |
| `value()` / `checksum()` | Returns the current CRC. |
| `static calculate(data, size)` | One-shot CRC calculation; O(size). |

## Usage Example
```cpp
#include "castle_ext/checksums/crc8.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "123456789";
    assert(castle::checksums::crc8::calculate(text, 9U) == 0xF4U);
    return 0;
}
```

## Constraints & Notes
Uses fixed-width arithmetic and constant memory. No heap allocation, exceptions, RTTI, virtual dispatch, or STL is used.
