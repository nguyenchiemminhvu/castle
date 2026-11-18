# Power-of-Two Test

## Overview
Checks whether an integral value is a non-zero power of two using the classic single-bit test. It exists for validating sizes that enable shift- and mask-based arithmetic in embedded code.

## Header
`#include "castle/math/is_power_of_two.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> bool is_power_of_two(T value) noexcept` | Returns `true` when `value` is non-zero and has exactly one bit set. Requires integral `T`. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/is_power_of_two.hpp"

constexpr bool a = castle::math::is_power_of_two(1U);
constexpr bool b = castle::math::is_power_of_two(16U);
```

See [`samples/sample_is_power_of_two.cpp`](../../samples/sample_is_power_of_two.cpp).

## Constraints & Notes
- `0` returns `false`.
- `1` returns `true`.
- The function is intended for non-negative inputs. Negative signed values are outside the supported contract, and the minimum signed value can overflow in the internal `value - 1` expression.
