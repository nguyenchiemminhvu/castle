# Integer Ceiling Division

## Overview
Computes mathematical ceiling division for integral values with a strictly positive denominator. It exists to give deterministic `ceil(numerator / denominator)` behavior without the overflow-prone `(a + b - 1) / b` pattern.

## Header
`#include "castle/math/ceil_div.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/error_handler.hpp`](../core/error_handler.md)
- [`castle/core/traits.hpp`](../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T ceil_div(T numerator, T denominator) noexcept` | Returns the mathematical ceiling of `numerator / denominator` for integral `T`. Exact divisions match ordinary C++ division; positive non-exact results round up. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/ceil_div.hpp"

constexpr int pages = castle::math::ceil_div(10, 4);
constexpr int signed_case = castle::math::ceil_div(-7, 3);
```

See [`samples/sample_ceil_div.cpp`](../../samples/sample_ceil_div.cpp).

## Constraints & Notes
- `denominator` must be greater than zero; zero or negative denominators violate the contract and trigger Castle's error handling when checks are enabled.
- Negative numerators are supported and round toward positive infinity, so `ceil_div(-7, 3)` returns `-2`.
- The implementation avoids the intermediate addition used by `(a + b - 1) / b`, which would overflow near the maximum representable positive numerator.
