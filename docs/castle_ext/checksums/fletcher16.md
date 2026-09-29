# Fletcher-16

## Overview
Implements Fletcher-16 using two modulo-255 accumulators over 8-bit blocks for lightweight, non-cryptographic error detection. It is an alias of the shared [Fletcher engine](fletcher_common.md); see also [Fletcher-32](fletcher32.md) and [Fletcher-64](fletcher64.md).

## Header
`#include "castle_ext/checksums/fletcher16.hpp"`

## Dependencies
- [`fletcher_common.md`](fletcher_common.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)

## Public API
| API | Description |
|---|---|
| `castle::checksums::fletcher16` | Stateful Fletcher-16 calculator. |
| `reset()` | Restores both accumulators. |
| `update(data, size)` | Processes bytes; O(size). |
| `value()` / `checksum()` | Returns the packed 16-bit checksum. |
| `static calculate(data, size)` | One-shot checksum; O(size). |

## Usage Example
```cpp
#include "castle_ext/checksums/fletcher16.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "123456789";
    assert(castle::checksums::fletcher16::calculate(text, 9U) == 0x1EDEU);
    return 0;
}
```

## Constraints & Notes
Blocks are single bytes, so byte order does not apply. Known vectors: `"abcde"` yields `0xC8F0`, `"abcdef"` yields `0x2057`, and `"abcdefgh"` yields `0x0627`.

Fletcher-16 is not a cryptographic authenticator. It uses constant memory and is suitable only where its error-detection properties meet the protocol requirements.
