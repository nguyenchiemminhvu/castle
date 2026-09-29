# Fletcher Common

## Overview
`castle::checksums::detail::basic_fletcher<UInt, LittleEndian>` is the width-generic streaming Fletcher engine behind [`fletcher16`](fletcher16.md), [`fletcher32`](fletcher32.md) and [`fletcher64`](fletcher64.md). The checksum width, block size and modulus are all derived at compile time from `UInt`; there is no heap, virtual dispatch, exception or lookup table.

## Header
`#include "castle_ext/checksums/fletcher_common.hpp"`

Applications normally include a width-specific header instead.

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)

## Template Parameters
| Parameter | Description |
|---|---|
| `UInt` | Checksum type: `uint16_t`, `uint32_t` or `uint64_t`. Any other width or a signed type fails a `static_assert`. |
| `LittleEndian` | `true` (default) assembles multi-byte blocks least-significant byte first; `false` reads them most-significant byte first. Has no effect on 16-bit checksums. |

## Derived Constants
| Constant | Value | Fletcher-16 | Fletcher-32 | Fletcher-64 |
|---|---|---|---|---|
| Block size | `sizeof(UInt) / 2` bytes | 1 | 2 | 4 |
| Modulus | `2^(8 * block size) - 1` | 255 | 65535 | 4294967295 |
| Result | `(sum2 << half_bits) \| sum1` | 16 bits | 32 bits | 64 bits |

## Public API
| API | Description |
|---|---|
| `value_type` | Alias of `UInt`. |
| `reset()` | Clears both sums and any buffered partial block. |
| `update(const uint8_t*, size)` | Consumes bytes; O(size). A null pointer is ignored. |
| `update(const void*, size)` | Byte-pointer convenience overload. |
| `value()` / `checksum()` | Returns the packed checksum. Does not modify the state, so it may be called between `update()` calls. |
| `static calculate(data, size)` | One-shot checksum for a `uint8_t*` or `void*` buffer; O(size). |

## Algorithm Notes
- Input is split into blocks of `sizeof(UInt) / 2` bytes. For each block: `sum1 = (sum1 + block) mod M`, then `sum2 = (sum2 + sum1) mod M`.
- Operands are always below `2 * M`, so reduction is a single conditional subtraction instead of a division.
- Bytes that do not yet fill a block are buffered in the object, so a message may be split into arbitrary chunks and still yield the same result as a one-shot call.
- A trailing partial block is zero-padded when `value()` is read. For big-endian blocks the padding is applied to the low-order bytes.
- Storage is `sum1`, `sum2`, one pending block of `UInt` and a byte counter.

## Usage Example
```cpp
#include "castle_ext/checksums/fletcher_common.hpp"
#include <assert.h>

int main()
{
    using fletcher_be32 = castle::checksums::detail::basic_fletcher<uint32_t, false>;

    const uint8_t text[] = "abcde";
    assert(fletcher_be32::calculate(text, 5U) == 0x4FF029C7UL);
    return 0;
}
```

## Constraints & Notes
Fletcher checksums are non-cryptographic. Because a block equal to the modulus is congruent to zero, an all-ones block and an all-zero block are indistinguishable; this is inherent to the algorithm.

The byte order selected by `LittleEndian` is a protocol choice, not a host-endianness setting: results are identical on little- and big-endian targets.
