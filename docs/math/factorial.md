# Factorial

## Overview
Provides compile-time and runtime factorial helpers based on `castle::size_type`. It exists for small deterministic combinatorics and table sizing without bringing in the STL.

## Header
`#include "castle/math/factorial.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/core/types.hpp`](../core/types.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <castle::size_type N> struct factorial` | Compile-time factorial metafunction. `factorial<N>::value` stores `N!` as `castle::size_type`. Template-instantiation depth is `O(N)`. |
| `castle::size_type factorial_v(castle::size_type n)` | Iterative `constexpr` runtime helper that returns `n!` as `castle::size_type`. Performs `O(n)` multiplications. |

## Usage Example
```cpp
#include "castle/math/factorial.hpp"

constexpr castle::size_type fixed = castle::math::factorial<5>::value;
constexpr castle::size_type runtime = castle::math::factorial_v(4);
```

See [`samples/sample_factorial.cpp`](../../samples/sample_factorial.cpp).

## Constraints & Notes
- `factorial<0>::value` and `factorial_v(0)` both return `1`.
- Results are stored in `castle::size_type`; no overflow detection is performed.
- Once the factorial exceeds the representable range of `size_type`, arithmetic follows that unsigned type's normal wraparound behavior.
