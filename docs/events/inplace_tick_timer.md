# In-place tick timer

## Overview

Tick-driven timer that stores its callback in-place and can be polled from an application main loop.

## Header

```cpp
#include "castle/events/inplace_tick_timer.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Timer type | Fixed callback storage plus period/deadline state. |
| Start/stop/tick | Arm a timer, stop it, and feed tick/time progression. |
| Callback execution | Invoke the callable when the configured period expires. |

## Example

```cpp
// Create a timer with a fixed callback buffer and call its tick/poll API from the system loop.
```

## Embedded notes

- No timer thread is created.
- Integrate it with a monotonic tick source for predictable timing.
