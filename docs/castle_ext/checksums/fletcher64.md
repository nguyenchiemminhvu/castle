# Fletcher-64

## Overview
Implements Fletcher-64 using two modulo-(2^32 - 1) accumulators over 32-bit blocks for lightweight, non-cryptographic error detection. It is an alias of the shared [Fletcher engine](fletcher_common.md).

## Header
`#include "castle_ext/checksums/fletcher64.hpp"`

## Dependencies
- [`fletcher_common.md`](fletcher_common.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::fletcher64` | Stateful Fletcher-64 calculator with little-endian blocks. |
| `castle::checksums::basic_fletcher64<LittleEndian>` | Same calculator with selectable 32-bit block byte order. |
| `reset()` | Restores both accumulators and discards buffered bytes. |
| `update(data, size)` | Processes bytes; O(size). |
| `value()` / `checksum()` | Returns the packed 64-bit checksum without consuming buffered bytes. |
| `static calculate(data, size)` | One-shot checksum; O(size). |

## Usage Example
```cpp
#include "castle_ext/checksums/fletcher64.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "abcdefgh";
    assert(castle::checksums::fletcher64::calculate(text, 8U) == 0x312E2B28CCCAC8C6ULL);

    using fletcher64_be = castle::checksums::basic_fletcher64<false>;
    assert(fletcher64_be::calculate(text, 8U) == 0x282B2E31C6C8CACCULL);
    return 0;
}
```

## Constraints & Notes
A trailing partial block (1 to 3 bytes) is zero-padded when the value is read. The default little-endian block order matches the common reference vectors, for example `"abcde"` yields `0xC8C6C527646362C6`.

Fletcher-64 is not a cryptographic authenticator. It uses constant memory.
