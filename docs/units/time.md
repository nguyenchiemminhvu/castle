# Time

## Overview
Time in `castle::units` *is* `castle::chrono::duration`; CASTLE has no second time type. This header names durations in the units vocabulary and owns the time literals (`_ns` to `_w`). Units arithmetic accepts durations directly: `meters / seconds` gives `meters_per_second`, and `meters / meters_per_second` gives a duration in seconds.

## Header
`#include "castle/units/time.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [duration](../chrono/duration.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `nanoseconds<Rep>`, `microseconds<Rep>`, `milliseconds<Rep>`, `seconds<Rep>`, `minutes<Rep>`, `hours<Rep>`, `days<Rep>`, `weeks<Rep>` | Aliases of `chrono::duration<Rep, scale>`. With `Rep = int64_t` they are exactly `chrono::milliseconds` and the other chrono aliases. |
| `literals::_ns`, `_us`, `_ms`, `_s`, `_min`, `_h`, `_d`, `_w` (integer) | Return the `int64_t` chrono aliases, e.g. `250_ms` is a `chrono::milliseconds`. |
| `literals::_ns` ... `_w` (floating) | Return `double` durations, e.g. `0.02_s` is a `seconds<double>`. |

## Usage Example
See `samples/units/time.cpp`.

```cpp
#include "castle/units/time.hpp"
#include "castle/units/distance.hpp"

using namespace castle::units::literals;

const castle::chrono::milliseconds timeout = 250_ms;
const auto speed = 100.0_m / 4_s;                        // 25 m/s
const castle::units::seconds<double> eta = 1.0_km / speed; // 40 s
```

## Constraints & Notes
- These literals replace the former `castle::chrono::literals::chrono_literals` (and `chrono/literals.hpp`). `castle/chrono/chrono.hpp` includes this header.
- Durations keep chrono's conversion rules: changing the period needs an explicit constructor, `chrono::duration_cast`, or `units::quantity_cast`.
- Units results with the dimension of time are always `duration<Rep, ratio<1>>` (seconds), converted from the derived scale.
- `inverse(duration)` gives a frequency. Use a floating representation, because integral reps truncate.
- Arithmetic is unchecked. Integer literals beyond the `int64_t` range wrap like any integer conversion.
