# `castle/bit/bit_reverse.h` — Bit / byte reversal

**Header:** `castle/bit/bit_reverse.h`
**Namespace:** `castle::bit`
**Sample:** [`samples/sample_bit.cpp`](../../samples/sample_bit.cpp) — `demo_bit_reverse()`

## Purpose

Reverses the ordering of bits (or bytes) inside an integral value. Bit
reversal is required by several CRC variants, FFT indexing, and
serial-protocol adapters where the wire transmits LSB-first while the
CPU sees MSB-first (or vice versa).

## Design notes

- Explicit `constexpr` overloads for `uint8_t`, `uint16_t`, `uint32_t`,
  `uint64_t`, implemented with the classic parallel-swap technique
  (nibble-swap, then 2-bit, then 1-bit; equivalent for 16/32/64 sizes).
- A generic dispatcher forwards other integral types via their unsigned
  representation.
- `reverse_bytes(v)` is a thin alias of `byte_swap(v)` from
  `bit_endian.h`, kept here for API symmetry.

## API

| Function                    | Description                        |
| --------------------------- | ---------------------------------- |
| `reverse_bits(uint8_t v)`   | Bit-reverse an 8-bit value         |
| `reverse_bits(uint16_t v)`  | Bit-reverse a 16-bit value         |
| `reverse_bits(uint32_t v)`  | Bit-reverse a 32-bit value         |
| `reverse_bits(uint64_t v)`  | Bit-reverse a 64-bit value         |
| `reverse_bits<T>(T v)`      | Generic dispatcher (SFINAE)        |
| `reverse_bytes<T>(T v)`     | Alias of `byte_swap(v)`            |

## Example — CRC-16 with LSB-first input

```cpp
#include <castle/bit/bit_reverse.h>
using namespace castle::bit;

uint16_t crc = 0xFFFF;
for (uint8_t b : payload)
{
    uint8_t bit_rev = reverse_bits(b);   // wire is LSB-first
    // …feed bit_rev into CRC lookup…
}
```

## See also

- `bit_endian.h` — `byte_swap` for pure byte-order conversions.
- `bit_count.h` — `popcount` is unaffected by bit ordering; useful as a
  sanity check.
