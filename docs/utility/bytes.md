# Bytes

## Overview
Provides alignment-independent signed and unsigned byte loads and writes with explicit endianness, fixed-width integer support, rotation helpers, and secure buffer wiping.

## Header
`#include "castle/utility/bytes.hpp"`

## Dependencies
- [compiler.md](../core/compiler.md)
- [types.md](../core/types.md)

## Public API
| API | Description |
|---|---|
| `read_le8/16/32/64` / `read_be8/16/32/64` | Reads fixed-width unsigned integers from little- or big-endian byte sequences. |
| `read_le8s/16s/32s/64s` / `read_be8s/16s/32s/64s` | Reads fixed-width signed integers from little- or big-endian byte sequences. |
| `write_le8/16/32/64` / `write_be8/16/32/64` | Writes fixed-width unsigned integers in little- or big-endian byte order. |
| `write_le8s/16s/32s/64s` / `write_be8s/16s/32s/64s` | Writes fixed-width signed integers in little- or big-endian byte order. |
| `rotl32`, `rotr32`, `rotr64` | Rotates fixed-width unsigned values; rotation count must be within the width and nonzero. |
| `secure_zero` | Clears a caller-owned buffer through volatile byte writes. |

## Usage Example
See `samples/utility/bytes.cpp`.

```cpp
uint8_t wire[4];
castle::write_be32(wire, 0x12345678U);
uint32_t value = castle::read_be32(wire);
```

## Constraints & Notes
- The caller must ensure each load/write buffer has enough bytes.
- These helpers do not allocate, throw, or depend on host byte order.
- Rotation counts must be in `1..31` for 32-bit helpers and `1..63` for `rotr64`.