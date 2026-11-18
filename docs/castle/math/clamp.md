# Clamp

## Overview
Restricts an arithmetic value to an inclusive lower and upper bound. It exists as a tiny deterministic alternative to STL algorithms for embedded code that needs range limiting without extra dependencies.

## Header
`#include "castle/math/clamp.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/error_handler.hpp`](../core/error_handler.md)
- [`castle/core/traits.hpp`](../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T clamp(T value, T low, T high) noexcept` | Returns `low` when `value < low`, `high` when `value > high`, otherwise `value`. Requires arithmetic `T`. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/clamp.hpp"

constexpr int duty = castle::math::clamp(125, 0, 100);
constexpr int centered = castle::math::clamp(-5, -2, 2);
```

See [`samples/sample_clamp.cpp`](../../samples/sample_clamp.cpp).

## Constraints & Notes
- `low` must be less than or equal to `high`; violating that precondition triggers Castle's error handling when checks are enabled.
- The bounds are inclusive.
- No range normalization or saturation state is stored; the function is a pure `O(1)` selection.
