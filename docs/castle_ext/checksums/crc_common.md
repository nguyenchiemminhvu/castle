# CRC Common

## Overview
`castle::checksums::detail::basic_crc<UInt, Polynomial, Initial, FinalXor, ReflectInput, ReflectOutput, Width>` is the width-generic, table-free streaming CRC engine behind [`crc8`](crc8.md), [`crc16`](crc16.md), [`crc24q`](crc24q.md), [`crc32`](crc32.md) and [`crc64`](crc64.md). Each concrete algorithm is an alias with the Rocksoft-model parameters filled in. There is no heap, virtual dispatch, exception or lookup table; each input byte is folded in with eight shift/XOR steps.

## Header
`#include "castle_ext/checksums/crc_common.hpp"`

Applications normally include a width-specific header instead.

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)

## Template Parameters
| Parameter | Description |
|---|---|
| `UInt` | Unsigned register type; also the `value_type` of the checksum. |
| `Polynomial` | Generator polynomial in normal (non-reflected) form. |
| `Initial` | Initial register value (see the note on reflected input below). |
| `FinalXor` | Value XOR-ed into the result when it is read. |
| `ReflectInput` | `true` processes each input byte least-significant bit first. |
| `ReflectOutput` | `true` reflects the result before the final XOR. |
| `Width` | CRC width in bits, `8` ... `sizeof(UInt) * 8`. Defaults to the full width of `UInt`, which keeps `crc8`, `crc16`, `crc32` and `crc64` unchanged. A smaller width (for example `24` in a `uint32_t`) keeps the register and the result masked to `Width` bits. |

`static_assert`s reject a `Width` outside `8` ... `sizeof(UInt) * 8` and a `Polynomial`, `Initial` or `FinalXor` that does not fit in `Width` bits.

## Public API
| API | Description |
|---|---|
| `value_type` | Alias of `UInt`. |
| `basic_crc()` | Starts from `Initial`; O(1). |
| `reset()` | Restores `Initial`; O(1). |
| `update(const uint8_t*, size)` | Consumes bytes; O(size). A null pointer is ignored. |
| `update(const void*, size)` | Byte-pointer convenience overload. |
| `value()` / `checksum()` | Returns the finalized checksum. Does not modify the state, so it may be called between `update()` calls; O(1). |
| `static calculate(data, size)` | One-shot checksum for a `uint8_t*` or `void*` buffer; a null pointer yields the CRC of an empty message; O(size). |

## Algorithm Notes
- Non-reflected input: the byte is XOR-ed into the top eight bits of the `Width`-bit register, then eight shift steps are applied, masking to `Width` bits after every shift.
- Reflected input: the register is kept in reflected (LSB-first) form, so the byte is XOR-ed into the low bits and the reflected polynomial is used. Because the register is stored reflected, **`Initial` must be given bit-reversed over `Width` bits** when it is not a palindrome. CRC-32 (`0xFFFFFFFF`) and any zero initial value are unaffected; for example CRC-24/BLE (catalogue init `0x555555`) is declared with `Initial = 0xAAAAAA`.
- `ReflectOutput != ReflectInput` reflects the register across the full `Width` bits before the final XOR.

## Usage Example
```cpp
#include "castle_ext/checksums/crc_common.hpp"
#include <assert.h>

int main()
{
    // CRC-24/OPENPGP: 24-bit CRC in a uint32_t register.
    using crc24_openpgp = castle::checksums::detail::basic_crc<uint32_t,
                                                               static_cast<uint32_t>(0x864CFBU),
                                                               static_cast<uint32_t>(0xB704CEU),
                                                               static_cast<uint32_t>(0x000000U),
                                                               false,
                                                               false,
                                                               24U>;
    const uint8_t text[] = "123456789";
    assert(crc24_openpgp::calculate(text, 9U) == 0x21CF02U);
    return 0;
}
```

See [`samples/castle_ext/checksums/crc24q.cpp`](../../../samples/castle_ext/checksums/crc24q.cpp), which also uses a custom alias.

## Constraints & Notes
CRC variants are not interchangeable: every parameter must match the protocol specification. CRCs are non-cryptographic error-detection codes. The engine processes one bit at a time, favouring code size and determinism over throughput.
