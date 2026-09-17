# Integer Floor Division

## Overview
Computes mathematical floor division for integral values with a strictly positive denominator. It exists because ordinary C++ integer division truncates toward zero, which is often wrong for negative coordinates and signed bucket mapping.

## Header
`#include "castle/math/floor_div.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/error_handler.hpp`](../core/error_handler.md)
- [`castle/core/traits.hpp`](../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T floor_div(T numerator, T denominator) noexcept` | Returns the mathematical floor of `numerator / denominator` for integral `T`. Exact divisions match ordinary C++ division; negative non-exact results round down. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/floor_div.hpp"

constexpr int left_cell = castle::math::floor_div(-7, 3);
constexpr int exact_cell = castle::math::floor_div(12, 3);
```

See [`samples/sample_floor_div.cpp`](../../samples/sample_floor_div.cpp).

## Constraints & Notes
- `denominator` must be greater than zero; zero or negative denominators violate the contract and trigger Castle's error handling when checks are enabled.
- Negative numerators are supported and round toward negative infinity, so `floor_div(-7, 3)` returns `-3`.
- This is useful for deterministic grid, tile, and fixed-point index calculations that must preserve mathematical ordering around zero.
