# Bit

## Overview
Umbrella header that pulls in every Castle bit-manipulation component. Use it when one translation unit needs several bit helpers and a single include is more convenient than selecting individual headers.

## Header
`#include "castle/bit/bit.hpp"`

## Dependencies
- [bit_core](bit_core.md)
- [bit_count](bit_count.md)
- [bit_mask](bit_mask.md)
- [bit_math](bit_math.md)
- [bit_reverse](bit_reverse.md)
- [bit_rotate](bit_rotate.md)
- [bit_utils](bit_utils.md)
- [flags](flags.md)

## Public API
| Signature | Description |
|---|---|
| `#include "castle/bit/bit.hpp"` | Includes the complete public API from the eight bit component headers listed above. No additional declarations are introduced by this umbrella header. |

## Usage Example
```cpp
#include "castle/bit/bit.hpp"

uint8_t value = 0;
value = castle::bit::set(value, 3U);
const uint8_t rotated = castle::bit::rotate_left(value, 1U);
castle::bit::flags<uint8_t, 0x0FU> status(rotated);
```
See `samples/sample_bit.cpp` for a complete example.

## Constraints & Notes
- Header-only and allocation-free.
- Behavior comes entirely from the included component headers.
- Prefer the narrower component header when include surface matters more than convenience.
