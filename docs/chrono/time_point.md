# Time Point

## Overview
Represents an instant on a specific clock timeline and provides conversions, arithmetic with durations, and comparisons between time points on the same clock.

## Header
`#include "castle/chrono/time_point.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Error Handler](../core/error_handler.md)
- [Traits](../core/traits.md)
- [Duration](duration.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename Clock, typename Duration> class time_point` | Core clock-relative instant type with constructors, `time_since_epoch()`, `min()`, `max()`, increment/decrement, and compound arithmetic operators. All operations are constant time. |
| `time_point_cast(...)` | Converts a time point to a different duration precision while preserving the clock type. Constant time. |
| `floor`, `ceil`, `round` | C++17-style rounding helpers for time points. |
| `operator+(time_point, duration)`, `operator+(duration, time_point)` | Advance a time point by a duration. |
| `operator-(time_point, duration)` | Move a time point backward by a duration. |
| `operator-(time_point, time_point)` | Compute elapsed duration between two time points on the same clock. |
| Comparison operators `==`, `!=`, `<`, `<=`, `>`, `>=` | Compare time points on the same clock after conversion to a common duration type. |

## Usage Example
See `samples/sample_time_point.cpp`.

```cpp
#include "castle/chrono/time_point.hpp"

struct test_clock
{
    using duration = castle::chrono::milliseconds;
};

int main()
{
    using point = castle::chrono::time_point<test_clock, castle::chrono::milliseconds>;
    point start(castle::chrono::milliseconds(100));
    point deadline = start + castle::chrono::milliseconds(25);
    (void)deadline;
    return 0;
}
```

## Constraints & Notes
- Tick resolution is controlled by the `Duration` template argument.
- A time point is meaningful only relative to its `Clock` type and epoch.
- Wall-clock versus monotonic behavior comes from the associated clock, not from `time_point` itself.
- Casting and arithmetic inherit duration truncation and overflow behavior.
