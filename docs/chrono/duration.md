# Duration

## Overview
Defines Castle's fixed-ratio time quantity template and the supporting casts, operators, aliases, and rounding helpers used throughout the chrono layer.

## Header
`#include "castle/chrono/duration.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Error Handler](../core/error_handler.md)
- [Traits](../core/traits.md)
- [Type Ranges](../core/type_ranges.md)
- [GCD](../math/gcd.md)
- [LCM](../math/lcm.md)
- [Ratio](../math/ratio.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename Ratio> struct is_valid_period` | Reports whether a ratio has positive numerator and denominator. |
| `template <typename Rep> struct duration_values` | Supplies `zero()`, `min()`, and `max()` for a representation type. |
| `template <typename Rep, typename Period> class duration` | Core time-quantity type with constructors, `count()`, `zero()`, `min()`, `max()`, increment/decrement, and compound arithmetic operators. All operations are constant time. |
| `template <typename ToDuration> duration_cast(...)` | Converts between duration periods. Constant time. |
| `using nanoseconds`, `microseconds`, `milliseconds`, `seconds`, `minutes`, `hours`, `days`, `weeks` | Common `int64_t` duration aliases. |
| Unary operators `+`, `-` | Identity and negation for durations. |
| Binary operators `+`, `-`, `*`, `/`, `%` | Duration arithmetic with other durations or scalars. |
| Comparison operators `==`, `!=`, `<`, `<=`, `>`, `>=` | Cross-period comparisons after conversion to a common period. |
| `floor`, `ceil`, `round`, `abs` | C++17-style rounding and absolute-value helpers for durations. |

## Usage Example
See `samples/sample_duration.cpp`.

```cpp
#include "castle/chrono/duration.hpp"

int main()
{
    castle::chrono::milliseconds period(10);
    const auto fine = castle::chrono::duration_cast<castle::chrono::microseconds>(period);
    const auto total = period + castle::chrono::milliseconds(5);
    (void)fine;
    (void)total;
    return 0;
}
```

## Constraints & Notes
- Tick resolution is controlled entirely by the `Period` ratio.
- A duration has no wall-clock or monotonic meaning until it is paired with a clock or time point.
- Integer period conversions truncate toward zero.
- Arithmetic and conversions are unchecked and may overflow the chosen representation type.
- Modulo operators require an underlying representation that supports `%`.
