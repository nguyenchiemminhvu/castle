# Fletcher-32

## Overview
Implements Fletcher-32 using two modulo-65535 accumulators over 16-bit blocks for lightweight, non-cryptographic error detection. It is an alias of the shared [Fletcher engine](fletcher_common.md).

## Header
`#include "castle_ext/checksums/fletcher32.hpp"`

## Dependencies
- [`fletcher_common.md`](fletcher_common.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::fletcher32` | Stateful Fletcher-32 calculator with little-endian blocks. |
| `castle::checksums::basic_fletcher32<LittleEndian>` | Same calculator with selectable 16-bit block byte order. |
| `reset()` | Restores both accumulators and discards a buffered odd byte. |
| `update(data, size)` | Processes bytes; O(size). |
| `value()` / `checksum()` | Returns the packed 32-bit checksum without consuming a buffered odd byte. |
| `static calculate(data, size)` | One-shot checksum; O(size). |

## Usage Example
```cpp
#include "castle_ext/checksums/fletcher32.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "abcde";
    assert(castle::checksums::fletcher32::calculate(text, 5U) == 0xF04FC729UL);

    using fletcher32_be = castle::checksums::basic_fletcher32<false>;
    assert(fletcher32_be::calculate(text, 5U) == 0x4FF029C7UL);
    return 0;
}
```

## Constraints & Notes
A trailing odd byte is zero-padded when the value is read. Choose the block byte order your protocol specifies; the default (little-endian) matches the common reference vectors, for example `"abcde"` yields `0xF04FC729`.

Fletcher-32 is not a cryptographic authenticator. It uses constant memory.
