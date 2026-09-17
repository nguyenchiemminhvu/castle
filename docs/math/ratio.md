# Ratio

## Overview
Compile-time rational-number facility for reduced fractions, ratio arithmetic, comparisons, and Castle's SI/time-unit aliases.

## Header
`#include "castle/math/ratio.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md)
- [gcd](gcd.md)

## Public API
| API | Description |
|---|---|
| `template <intmax_t Numerator, intmax_t Denominator = 1> struct castle::math::ratio` | Reduces a fraction to canonical `num` / `den` form with a non-negative denominator. |
| `ratio_add`, `ratio_subtract`, `ratio_multiply`, `ratio_divide` and `_t` aliases | Perform compile-time ratio arithmetic and expose the reduced result as `type` or alias form. |
| `ratio_equal`, `ratio_not_equal`, `ratio_less`, `ratio_less_equal`, `ratio_greater`, `ratio_greater_equal` | Compile-time comparisons over reduced ratios. |
| `castle::atto`, `castle::femto`, `castle::pico`, `castle::nano`, `castle::micro`, `castle::milli`, `castle::centi`, `castle::deci`, `castle::deca`, `castle::hecto`, `castle::kilo`, `castle::mega`, `castle::giga` | Common SI scaling aliases. |
| `castle::seconds`, `castle::minutes`, `castle::hours`, `castle::days`, `castle::weeks` | Common time-period aliases. |

## Usage Example
See `samples/sample_ratio.cpp` for a complete example.

```cpp
using half = castle::math::ratio<2, 4>;
using third = castle::math::ratio<1, 3>;
using sum = castle::math::ratio_add_t<half, third>;
```

## Constraints & Notes
- `Denominator` must not be zero.
- Ratios are reduced at compile time, and denominators are normalized positive.
- Arithmetic and comparisons use `intmax_t` cross-products, so very large operands can overflow during template evaluation.
