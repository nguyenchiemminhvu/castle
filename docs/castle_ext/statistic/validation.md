# validation

## Overview
Shared sample validation and result-type selection for `castle::statistic`. `result_t<T>` chooses the floating-point type used for results (the sample type itself for floating-point samples, `double` for integers). `is_finite()` and `is_probability()` give every component one definition of a usable sample and a valid probability, without `<math.h>`.

## Header
`#include "castle_ext/statistic/validation.hpp"`

## Dependencies
- [compiler](../../castle/core/compiler.md)
- [traits](../../castle/core/traits.md)
- [type_ranges](../../castle/core/type_ranges.md)

## Public API
| API | Description | Complexity (time / stack) |
| --- | --- | --- |
| `result_t<T>` | Alias: `T` for `float`, `double`, `long double`; `double` for integral `T`. | compile time |
| `is_finite(value)` (floating `T`) | `true` when `lowest() <= value <= max()`. NaN, `+inf` and `-inf` return `false`. `constexpr`, `noexcept`. | O(1) / O(1) |
| `is_finite(value)` (integral `T`) | Always `true`. `constexpr`, `noexcept`. | O(1) / O(1) |
| `is_probability(value)` (floating `T`) | `true` when `0 <= value <= 1` (closed interval). NaN returns `false`. `constexpr`, `noexcept`. | O(1) / O(1) |

No function returns a `castle::status`; callers translate a `false` result into `status::invalid_argument`.

## Usage Example
See [`samples/castle_ext/statistic/validation.cpp`](../../../samples/castle_ext/statistic/validation.cpp).

```cpp
using result = castle::statistic::result_t<uint32_t>; // double
const bool usable = castle::statistic::is_finite(1.5);
const bool valid_level = castle::statistic::is_probability(0.95);
```

## Constraints & Notes
- Header-only, no allocation, no exceptions, no `<math.h>`.
- `is_finite()` relies on NaN failing every ordered comparison, so it must not be compiled with `-ffast-math` or `-ffinite-math-only`.
- `is_probability()` is only provided for floating-point types.
- Stateless and therefore thread-safe.
