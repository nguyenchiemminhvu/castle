# Greatest Common Divisor

## Overview
Compile-time Euclidean greatest-common-divisor support for `intmax_t` template arguments. It exists mainly for ratio reduction and period calculations elsewhere in Castle.

## Header
`#include "castle/math/gcd.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/math/abs.hpp`](abs.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <intmax_t A, intmax_t B> struct gcd` | Compile-time Euclidean algorithm. `gcd<A, B>::value` stores the terminating divisor as `intmax_t`. Instantiation depth is proportional to the length of the remainder chain. |

## Usage Example
```cpp
#include "castle/math/gcd.hpp"

constexpr intmax_t divisor = castle::math::gcd<84, 30>::value;
```

See [`samples/sample_gcd.cpp`](../../samples/sample_gcd.cpp).

## Constraints & Notes
- `gcd<A, 0>::value` is `A`, so `gcd<0, 0>::value` is `0`.
- This header does not normalize the sign of its template arguments; negative operands can therefore produce a negative result.
- For canonical positive GCD results, instantiate it with non-negative values.
