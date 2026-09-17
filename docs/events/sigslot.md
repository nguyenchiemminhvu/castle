# Signal/slot connections

## Overview

Signal/slot facility for connecting multiple handlers to one signal.

## Header

```cpp
#include "castle/events/sigslot.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Signal/slot types | Connect callbacks to a signal. |
| Emit | Invoke connected slots in registration order where defined. |
| Disconnect/subscription | Remove a connection explicitly. |

## Example

```cpp
// Connect a slot to a signal, emit the signal, then disconnect when the owner goes away.
```

## Embedded notes

- Make connection lifetime explicit to avoid dangling receiver objects.
