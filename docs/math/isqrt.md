# Integer Square Root

## Overview
Returns the floor of the square root for integral values using a binary search. It exists for deterministic integer geometry and indexing code that must avoid floating-point square-root calls.

## Header
`#include "castle/math/isqrt.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/error_handler.hpp`](../core/error_handler.md)
- [`castle/core/traits.hpp`](../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T isqrt(T value) noexcept` | Unsigned overload. Returns the largest `r` such that `r * r <= value`. Uses `value / middle` to avoid multiplication overflow during the search. `O(log value)`. |
| `template <typename T> T isqrt(T value) noexcept` | Signed overload. Requires a non-negative input, then forwards to the corresponding unsigned calculation. `O(log value)`. |

## Usage Example
```cpp
#include "castle/math/isqrt.hpp"

constexpr unsigned int root16 = castle::math::isqrt(16U);
constexpr int root15 = castle::math::isqrt(15);
```

See [`samples/sample_isqrt.cpp`](../../samples/sample_isqrt.cpp).

## Constraints & Notes
- Returns `0` for `0` and `1` for `1`.
- Signed inputs must be non-negative; negative values violate the contract and trigger Castle's error handling when checks are enabled.
- The search compares `middle <= value / middle` instead of `middle * middle <= value`, avoiding multiplication overflow.
