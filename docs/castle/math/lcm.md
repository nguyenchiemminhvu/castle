# Least Common Multiple

## Overview
Compile-time least-common-multiple support for positive `intmax_t` template arguments. It exists mainly for ratio denominators and chrono period calculations that need a shared multiple at compile time.

## Header
`#include "castle/math/lcm.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/math/gcd.hpp`](gcd.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <intmax_t A, intmax_t B> struct lcm` | Compile-time least-common-multiple metafunction. `lcm<A, B>::value` stores `(A / gcd<A, B>::value) * B` as `intmax_t`. |

## Usage Example
```cpp
#include "castle/math/lcm.hpp"

constexpr intmax_t common = castle::math::lcm<12, 15>::value;
```

See [`samples/sample_lcm.cpp`](../../samples/sample_lcm.cpp).

## Constraints & Notes
- `A` and `B` must both be strictly positive; zero or negative values fail the `static_assert`s in the template.
- Dividing before multiplying reduces intermediate growth but does not eliminate overflow risk for large results.
- No runtime overload is provided in this header.
