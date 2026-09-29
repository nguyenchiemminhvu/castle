# XOR Checksum

## Overview
Computes the byte-wise XOR of an input range, with an optional initial accumulator value for simple framing protocols.

## Header
`#include "castle_ext/checksums/xor.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `xor_checksum(initial)` | Constructs a calculator with an initial accumulator. |
| `reset(initial)` | Reinitializes the accumulator. |
| `update(data, size)` | XORs bytes into the accumulator; O(size). |
| `value()` / `checksum()` | Returns the current checksum byte. |
| `static calculate(data, size)` | One-shot calculation with initial value zero. |
| `static calculate(data, size, initial)` | One-shot calculation with a caller-selected initial value. |

## Usage Example
```cpp
#include "castle_ext/checksums/xor.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "123456789";
    assert(castle::checksums::xor_checksum::calculate(text, 9U) == 0x31U);
    return 0;
}
```

## Constraints & Notes
Very small and deterministic, but intentionally weak. Never use XOR checksum as a security primitive.
