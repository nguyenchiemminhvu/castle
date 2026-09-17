# Chrono umbrella header

## Overview

Convenience header that pulls in Castle durations, time points, clocks, and chrono literals.

## Header

```cpp
#include "castle/chrono/chrono.h"
```

**Namespace:** `castle::chrono`

## Main API

| API | Purpose |
|---|---|
| `duration` | Fixed-ratio time quantity. |
| `time_point` | Clock-relative point in time. |
| `system_clock` / `steady_clock` | Platform-backed clocks. |
| Chrono literals | Convenient `_ns`, `_us`, `_ms`, `_s`, and related literals where enabled. |

## Example

```cpp
using namespace castle::chrono_literals;
auto timeout = 250_ms;
```

## Embedded notes

- Use the smaller header when include-time matters.
