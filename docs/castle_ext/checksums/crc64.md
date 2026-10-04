# CRC-64

## Overview
Implements CRC-64/ECMA-182 with polynomial `0x42F0E1EBA9EA3693`, initial value `0x0000000000000000`, no final XOR, and non-reflected input/output.

## Header
`#include "castle_ext/checksums/crc64.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::crc64` | Stateful CRC-64 calculator. |
| `reset()` | Restores the initial CRC. |
| `update(data, size)` | Updates the checksum; O(size). |
| `value()` / `checksum()` | Returns the current 64-bit checksum. |
| `static calculate(data, size)` | One-shot calculation; O(size). |

## Usage Example
```cpp
#include "castle_ext/checksums/crc64.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "123456789";
    assert(castle::checksums::crc64::calculate(text, 9U) == 0x6C40DF5F0B497347ULL);
    return 0;
}
```

## Constraints & Notes
Constant memory and deterministic arithmetic. CRC-64 variants must not be substituted for each other without matching the wire/file specification.
