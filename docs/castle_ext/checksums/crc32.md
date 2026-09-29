# CRC-32

## Overview
Implements CRC-32/ISO-HDLC with polynomial `0x04C11DB7`, reflected input/output, and initial/final XOR `0xFFFFFFFF`.

## Header
`#include "castle_ext/checksums/crc32.hpp"`

## Dependencies

- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::crc32` | Stateful CRC-32 calculator. |
| `reset()` | Restores the initial CRC state. |
| `update(data, size)` | Adds bytes; O(size). |
| `value()` / `checksum()` | Returns the current CRC. |
| `static calculate(data, size)` | One-shot calculation; O(size). |

## Usage Example
```cpp
#include "castle_ext/checksums/crc32.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "123456789";
    assert(castle::checksums::crc32::calculate(text, 9U) == 0xCBF43926U);
    return 0;
}
```

## Constraints & Notes
The implementation uses a compact bitwise update instead of a lookup table, trading some throughput for small fixed code/data footprint. Complexity is O(size).
