# MD5

## Overview
Provides a streaming RFC 1321 MD5 message digest for compatibility with legacy protocols and known data formats. MD5 is not collision-resistant.

## Header
`#include "castle_ext/crypto/hash/md5.hpp"`

## Dependencies
- [`../../../core/compiler.md`](../../../core/compiler.md)
- [`../../../core/error_handler.md`](../../../core/error_handler.md)
- [`../../../core/types.md`](../../../core/types.md)
- [`../../../error/status.md`](../../../error/status.md)
- [`../../../container/array.md`](../../../container/array.md)
- [`../../../../include/castle/utility/bytes.hpp`](../../../../include/castle/utility/bytes.hpp)

## Public API
| API | Description |
|---|---|
| `digest_size`, `block_size` | Compile-time constants: 16 and 64 bytes. |
| `reset()` | Starts a new digest. |
| `update(data, size)` | Adds bytes; returns `castle::status`. |
| `digest()` | Returns a fixed `castle::container::array<uint8_t, 16>` snapshot. |
| `final(output, capacity)` | Copies the digest to caller-owned storage. |
| `static calculate(data, size)` | One-shot digest calculation. |

## Usage Example
```cpp
#include "castle_ext/crypto/hash/md5.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "abc";
    const auto digest = castle::crypto::hash::md5::calculate(text, 3U);
    assert(digest[0] == 0x90U && digest[15] == 0x72U);
    return 0;
}
```

## Constraints & Notes
Uses fixed internal storage and no heap. MD5 must not be selected for new collision-resistant security designs; prefer SHA-256 or SHA-512.
