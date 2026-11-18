# Mod

## Overview
Integral normalization helpers for positive modulo arithmetic and periodic wrapping into a half-open interval.

## Header
`#include "castle/math/mod.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| API | Description |
|---|---|
| `template <typename T> T castle::math::positive_mod(T value, T modulus)` | Returns a remainder in `[0, modulus)` for integral types. |
| `template <typename T> T castle::math::wrap(T value, T low, T high)` | Wraps an integral value into the half-open interval `[low, high)`. |

## Usage Example
See `samples/sample_mod.cpp` for a complete example.

```cpp
const int phase = castle::math::positive_mod(-1, 8);
const int wrapped = castle::math::wrap(14, 10, 13);
```

## Constraints & Notes
- `positive_mod()` requires `modulus > 0`.
- `wrap()` requires `high > low` and treats `high` as exclusive.
- `wrap()` widens common 8/16/32-bit intermediate arithmetic before subtracting interval endpoints.
