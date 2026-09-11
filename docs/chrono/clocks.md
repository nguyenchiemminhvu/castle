# Embedded clocks

## Overview

Defines `system_clock`, `steady_clock`, and `high_resolution_clock`, while leaving the actual platform timer source to the application.

## Header

```cpp
#include "castle/chrono/clocks.h"
```

**Namespace:** `castle::chrono`

## Main API

| API | Purpose |
|---|---|
| `system_clock::now()` | Returns current wall-clock time using the configured platform hook. |
| `steady_clock::now()` | Returns monotonic time suitable for timeouts and scheduling. |
| `high_resolution_clock` | Alias of `system_clock` in this implementation. |
| `to_time_t()` / `from_time_t()` | Convert the system clock to and from C `time_t`. |
| Clock configuration macros | Select clock period and provide platform `now()` hooks or expressions. |

## Example

```cpp
auto start = castle::chrono::steady_clock::now();
// ...
auto elapsed = castle::chrono::steady_clock::now() - start;
```

## Embedded notes

- The library does not select an RTOS or HAL.
- Applications should provide monotonic hardware-backed behavior for `steady_clock`.
- Default periods are nanoseconds unless overridden.
