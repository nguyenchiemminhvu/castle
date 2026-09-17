# Event dispatcher

## Overview

Dispatches application events to registered handlers using fixed-capacity Castle facilities.

## Header

```cpp
#include "castle/events/event_dispatcher.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| `event_dispatcher` | Main dispatcher type and its event registration/dispatch operations. |
| Event registration | Connect handlers to event identifiers or types. |
| Dispatch/post | Send an event to the matching listeners. |
| Capacity/state | Fixed limits and bounded failure reporting. |

## Example

```cpp
// Configure a dispatcher, register handlers, then dispatch events.
// The exact event type is defined by the header template parameters.
```

## Embedded notes

- No heap-backed event queue is required by the fixed-capacity design.
