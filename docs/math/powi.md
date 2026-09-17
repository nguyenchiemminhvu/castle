
# Powi

## Overview
Exponentiation-by-squaring helper for raising a value to a non-negative integer power with logarithmic multiplication count.

## Header
`#include "castle/math/powi.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `template <typename T> T castle::math::powi(T base, unsigned exponent)` | Returns `base^exponent` using exponentiation by squaring. |

## Usage Example
See `samples/sample_powi.cpp` for a complete example.

```cpp
const int gain = castle::math::powi(3, 4U);
const float squared = castle::math::powi(1.5f, 2U);
```

## Constraints & Notes
- Complexity is `O(log exponent)`.
- The function is `constexpr` and deterministic.
- Arithmetic stays in `T`; there is no widening, saturation, or overflow detection.
