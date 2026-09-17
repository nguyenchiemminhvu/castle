# Signal IPC event

## Overview

Event primitive for sending lightweight signals between embedded components.

## Header

```cpp
#include "castle/events/signal_ipc_event.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Signal event type | Represent an IPC notification and its routing state. |
| Emit/receive operations | Send and handle the event using bounded listener storage. |

## Example

```cpp
// Use the signal IPC event API when a simple notification is preferable to a queue.
```

## Embedded notes

- The exact transport is application-owned; the Castle type provides the bounded event abstraction.
