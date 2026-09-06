# In-place signal IPC event

## Overview

In-object event/signal primitive for lightweight inter-component notification with bounded storage.

## Header

```cpp
#include "castle/events/inplace_signal_ipc_event.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Signal event type | Represents a small IPC event and its associated callback/signal handling. |
| Emit/handle | Send the signal and route it to registered consumers. |

## Example

```cpp
// Use the signal event type when a small in-place notification path is enough.
```

## Embedded notes

- Designed for embedded message flow without general-purpose dynamic event objects.
