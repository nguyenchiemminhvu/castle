# Time points

## Overview

Represents a point on a clock timeline and provides arithmetic with durations.

## Header

```cpp
#include "castle/chrono/time_point.h"
```

**Namespace:** `castle::chrono`

## Main API

| API | Purpose |
|---|---|
| `time_point<Clock, Duration>` | Clock-specific time point. |
| `time_since_epoch()` | Get the underlying duration. |
| `time_point_cast()` | Change the duration precision of a time point. |
| Arithmetic | Add/subtract durations; subtract two time points to get a duration. |
| Comparisons | Compare time points on the same clock. |

## Example

```cpp
using clock = castle::chrono::steady_clock;
auto deadline = clock::now() + castle::chrono::milliseconds(100);
```

## Embedded notes

- A time point is meaningful relative to its clock type.
- Use `steady_clock` for elapsed-time measurements.
