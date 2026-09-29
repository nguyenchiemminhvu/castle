# CRC-16

## Overview
Implements CRC-16/CCITT-FALSE with polynomial `0x1021`, initial value `0xFFFF`, no final XOR, and non-reflected input/output.

## Header
`#include "castle_ext/checksums/crc16.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::crc16` | Stateful CRC-16 calculator. |
| `reset()` | Restores the initial CRC state. |
| `update(data, size)` | Adds bytes; O(size). |
| `value()` / `checksum()` | Returns the current CRC. |
| `static calculate(data, size)` | One-shot calculation; O(size). |

## Usage Example
```cpp
#include "castle_ext/checksums/crc16.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "123456789";
    assert(castle::checksums::crc16::calculate(text, 9U) == 0x29B1U);
    return 0;
}
```

## Constraints & Notes
Deterministic and allocation-free. CRC-16 variants are not interchangeable, so this exact parameter set must match the protocol specification.
