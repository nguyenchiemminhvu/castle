# In-place signal/slot

## Overview

Fixed-capacity signal/slot connection primitive backed by in-place callbacks.

## Header

```cpp
#include "castle/events/inplace_sigslot.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Signal/slot types | Connect slots to a signal and invoke all active slots. |
| Subscription lifetime | Remove a connection explicitly or through the associated handle. |
| Capacity | Keep a compile-time upper bound on connections. |

## Example

```cpp
// Connect a slot, emit a signal, then release the slot subscription.
```

## Embedded notes

- Prefer explicit connection lifetime in interrupt/RTOS code.
