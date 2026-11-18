# Saturating

## Overview
Integral add/subtract helpers that clamp out-of-range results to the type limits instead of allowing wraparound.

## Header
`#include "castle/math/saturating.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md)
- [type_ranges](../core/type_ranges.md)

## Public API
| API | Description |
|---|---|
| `template <typename T> T castle::math::saturating_add(T a, T b)` | For unsigned types, clamps upward overflow to `max()`. For signed types, clamps to `min()`/`max()` on overflow. |
| `template <typename T> T castle::math::saturating_sub(T a, T b)` | For unsigned types, clamps underflow to `0`. For signed types, clamps to `min()`/`max()` when the exact difference is out of range. |

## Usage Example
See `samples/sample_saturating.cpp` for a complete example.

```cpp
const uint8_t hi = castle::math::saturating_add<uint8_t>(250U, 20U);
const int8_t lo = castle::math::saturating_sub<int8_t>(-120, 20);
```

## Constraints & Notes
- Only integral types are supported.
- Complexity is constant time with no heap allocation, exceptions, or hidden state.
- Saturation is often preferable to wraparound for control outputs and bounded accumulators.
