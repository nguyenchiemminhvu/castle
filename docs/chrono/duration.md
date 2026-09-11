# Duration

## Overview

Represents a quantity of time as a representation type plus a compile-time ratio. It is designed as a small STL-free alternative to `std::chrono::duration`.

## Header

```cpp
#include "castle/chrono/duration.h"
```

**Namespace:** `castle::chrono`

## Main API

| API | Purpose |
|---|---|
| `duration<Rep, Period>` | Core duration type. |
| `count()` | Return the stored representation. |
| `zero()` / `min()` / `max()` | Named duration values. |
| Arithmetic operators | Increment/decrement and arithmetic with durations and scalars. |
| `duration_cast<ToDuration>()` | Convert between time periods. |
| Standard aliases | `nanoseconds`, `microseconds`, `milliseconds`, `seconds`, `minutes`, `hours`, `days`, `weeks`. |

## Example

```cpp
castle::chrono::milliseconds timeout(250);
auto ticks = castle::chrono::duration_cast<castle::chrono::microseconds>(timeout);
```

## Embedded notes

- Period ratios come from Castle `math::ratio`.
- A duration does not allocate.
- Choose a smaller representation such as `uint32_t` when the required time range allows it.
