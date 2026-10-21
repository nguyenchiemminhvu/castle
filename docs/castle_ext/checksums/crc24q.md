# CRC-24Q

## Overview
Implements CRC-24Q with width 24, polynomial `0x864CFB`, initial value `0x000000`, no final XOR, and non-reflected input/output. It is the frame check sequence of RTCM 3.x messages (see [`rtcm.md`](../protocols/rtcm/rtcm.md)) and of SBAS L1/L5 messages. The 24-bit result is stored in the low bits of a `uint32_t`; the top byte is always zero.

`crc24q` is an alias of the width-generic engine described in [`crc_common.md`](crc_common.md). The algorithm is a table-free bitwise loop, so it uses no lookup table and no heap.

## Header
`#include "castle_ext/checksums/crc24q.hpp"`

## Dependencies
- [`crc_common.md`](crc_common.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::crc24q` | Stateful CRC-24Q calculator; `value_type` is `uint32_t`. |
| `crc24q()` | Starts from the initial register value `0`. |
| `reset()` | Restores the initial CRC state; O(1). |
| `update(const uint8_t*, size)` | Adds bytes; O(size). A null pointer is ignored. |
| `update(const void*, size)` | Byte-pointer overload for untyped buffers; O(size). |
| `value()` / `checksum()` | Returns the current CRC without modifying the state, so it may be called between `update()` calls; O(1). |
| `static calculate(const uint8_t*, size)` | One-shot calculation; a null pointer yields the CRC of an empty message (`0`); O(size). |
| `static calculate(const void*, size)` | One-shot byte-pointer overload; O(size). |

The standard check value of the ASCII message `123456789` is `0xCDE703`.

## Usage Example
```cpp
#include "castle_ext/checksums/crc24q.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "123456789";
    assert(castle::checksums::crc24q::calculate(text, 9U) == 0xCDE703U);

    castle::checksums::crc24q crc;
    crc.update(text, 4U);
    crc.update(text + 4U, 5U);
    assert(crc.value() == 0xCDE703U);
    return 0;
}
```

See [`samples/castle_ext/checksums/crc24q.cpp`](../../../samples/castle_ext/checksums/crc24q.cpp) for the full API.

## Constraints & Notes
Deterministic and allocation-free; not thread-safe when one instance is shared, but `calculate()` uses only local state. CRC-24Q is an error-detection code, not a cryptographic hash.

RTCM transmits the CRC most significant byte first and, because there is no final XOR, running the CRC over a message followed by its own big-endian CRC yields `0`. Different 24-bit CRCs (OpenPGP, FlexRay, BLE, ...) are not interchangeable with this parameter set.
