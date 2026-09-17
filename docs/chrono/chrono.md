# Chrono

## Overview
Convenience umbrella header that includes Castle durations, time points, clocks, and chrono literals in one place.

## Header
`#include "castle/chrono/chrono.hpp"`

## Dependencies
- [Duration](duration.md)
- [Time Point](time_point.md)
- [Clocks](clocks.md)
- [Literals](literals.md)

## Public API
| API | Description |
| --- | --- |
| `castle::chrono::duration` | Fixed-ratio time quantity template. |
| `castle::chrono::time_point` | Clock-relative instant template. |
| `castle::chrono::system_clock`, `steady_clock`, `high_resolution_clock` | Platform-backed clock types and alias. |
| `castle::chrono::literals::chrono_literals::*` | User-defined literals for common duration aliases. |

## Usage Example
See `samples/sample_chrono.cpp`.

```cpp
#include "castle/chrono/chrono.hpp"

using namespace castle::chrono::literals::chrono_literals;

int main()
{
    const auto timeout = 250_ms;
    const auto start = castle::chrono::steady_clock::now();
    const auto deadline = start + timeout;
    (void)deadline;
    return 0;
}
```

## Constraints & Notes
- This header adds no new semantics; tick resolution, monotonicity, and overflow behavior come from the included chrono components.
- `steady_clock` is the monotonic clock abstraction; `system_clock` is wall-clock based and may step backward or forward.
- Duration and time-point arithmetic is unchecked and may overflow the underlying representation.
