# Safe Cast

## Overview
This header provides explicit primitive conversion helpers that clamp or normalize values according to Castle's deterministic rules. It exists for protocol boundaries, register packing, and other embedded conversion points where silent narrowing must be controlled.

## Header
`#include "castle/utility/safe_cast.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [error_handler.md](../core/error_handler.md), [traits.md](../core/traits.md), [type_ranges.md](../core/type_ranges.md), [abs.md](../math/abs.md)

## Public API
| API | Description |
|---|---|
| `safe_cast` | Namespace-style class exposing named conversion helpers such as `int16_to_uint8` and `double_to_float`. |
| `SAFE_CAST<From, To>(value)` | Primary conversion entry point with Castle specializations for supported arithmetic conversions. |

## Usage Example
See `samples/sample_safe_cast.cpp`.

```cpp
uint8_t clipped = castle::SAFE_CAST<int16_t, uint8_t>(300);
bool enabled = castle::SAFE_CAST<float, bool>(1.0f);
```

## Constraints & Notes
- No heap allocation or exceptions.
- Integral narrowing clamps to the destination range when needed.
- Signed-to-unsigned conversions clamp negative inputs to zero.
- Floating-point to integer conversions clamp to the destination range and truncate toward zero after the cast.
- `SAFE_CAST` uses explicit specializations for the supported primitive conversion pairs.
