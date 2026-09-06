# GCC clock backend hook

## Overview

GCC-specific time source helpers used by Castle clocks when GCC clock macros are enabled.

## Header

```cpp
#include "castle/chrono/clock_variants/gcc_clock_variant.h"
```

**Namespace:** `castle::chrono::detail`

## Main API

| API | Purpose |
|---|---|
| `realtime_ns()` / `monotonic_ns()` | Backend helpers used by the GCC clock path when the corresponding macros are enabled. |

## Example

```cpp
// Usually consumed indirectly by castle::chrono::clocks.
```

## Embedded notes

- Application code should call `system_clock::now()` or `steady_clock::now()` instead of these backend helpers.
