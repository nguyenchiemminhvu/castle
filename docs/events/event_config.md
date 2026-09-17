# Event configuration

## Overview

Compile-time settings shared by the event and timer components.

## Header

```cpp
#include "castle/events/event_config.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Event configuration types/macros | Set capacities, callback limits, timer policies, or other compile-time event parameters used by the dispatcher family. |

## Example

```cpp
#include "castle/events/event_config.h"
```

## Embedded notes

- Keep configuration fixed and visible at compile time for deterministic resource usage.
