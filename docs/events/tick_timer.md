# Tick timer

## Overview

Timer primitive driven by a caller-supplied tick or chrono time source.

## Header

```cpp
#include "castle/events/tick_timer.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Timer type | Stores period/deadline and callback state. |
| Start/stop | Enable or disable a timer. |
| Tick/poll | Advance the timer and run the callback when due. |

## Example

```cpp
// Poll the timer from the application loop rather than creating a timer thread.
```

## Embedded notes

- The design is deterministic because no internal thread or heap queue is required.
