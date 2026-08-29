# `castle/bit/bit_endian.h` — Byte-order conversion

**Header:** `castle/bit/bit_endian.h`
**Namespace:** `castle::bit`
**Sample:** [`samples/sample_bit.cpp`](../../samples/sample_bit.cpp) — `demo_bit_endian()`

## Purpose

`byte_swap()` reverses the byte order of an integral value. This is the
building block used when marshaling values into/out of automotive
communication frames (CAN, LIN, FlexRay, UDS, DoIP, NMEA, UBX,
Ethernet/IP …) where the wire endianness rarely matches the host CPU.

## Design notes

- Explicit `constexpr` overloads for `uint8_t`, `uint16_t`, `uint32_t`,
  `uint64_t` — implemented as bit-shift/mask chains, so no compiler
  intrinsics or unaligned loads are required.
- A generic dispatcher forwards signed / other integral types through
  their unsigned counterpart, keeping semantics well-defined.
- `byte_swap(uint8_t)` returns the input unchanged (single byte, nothing
  to swap). Documented for completeness in generic code paths.

## API

| Overload                                | Returns    |
| --------------------------------------- | ---------- |
| `byte_swap(uint8_t v)`                  | `uint8_t`  |
| `byte_swap(uint16_t v)`                 | `uint16_t` |
| `byte_swap(uint32_t v)`                 | `uint32_t` |
| `byte_swap(uint64_t v)`                 | `uint64_t` |
| `byte_swap(T v)` (SFINAE-constrained)   | `T`        |

## Example — reading a big-endian CAN payload on a little-endian MCU

```cpp
#include <castle/bit/bit_endian.h>
using namespace castle::bit;

// Wire-format (network order) 16-bit value received from a CAN frame:
uint16_t wire = (payload[0] << 8) | payload[1];  // big-endian assembly
uint16_t host = byte_swap(wire);                 // now little-endian
```

## See also

- `bit_reverse.h` — `reverse_bytes` is an alias of `byte_swap`;
  `reverse_bits` reverses at the bit granularity instead.
