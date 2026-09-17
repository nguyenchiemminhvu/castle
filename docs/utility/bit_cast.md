# Bit Cast

## Overview
`castle::bit_cast` copies the raw bit pattern of one trivially copyable type into another same-size trivially copyable type. It exists so embedded code can inspect wire-format or register representations without aliasing tricks.

## Header
`#include "castle/utility/bit_cast.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [traits.md](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `bit_cast<To>(const From& source)` | Returns a `To` value with the same object representation as `source`. `To` and `From` must be trivially copyable and have equal size. |
| `CASTLE_HAS_BUILTIN_BIT_CAST` | Preprocessor flag indicating whether the current compiler provides `__builtin_bit_cast`. |

## Usage Example
See `samples/sample_bit_cast.cpp`.

```cpp
struct WireWord
{
    uint32_t value;
};

uint32_t raw = castle::bit_cast<uint32_t>(WireWord{0x12345678U});
```

## Constraints & Notes
- No dynamic allocation, exceptions, RTTI, or virtual dispatch.
- Uses a compiler builtin when available; otherwise falls back to a byte copy.
- Intended for representation-preserving casts only; it does not start or end object lifetimes.
