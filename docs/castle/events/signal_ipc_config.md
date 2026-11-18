# Signal IPC configuration

## Overview

Compile-time options shared by Castle’s signal/IPC event components.

## Header

```cpp
#include "castle/events/signal_ipc_config.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Configuration values | Set bounded signal/slot capacities and behavior for the IPC event family. |

## Example

```cpp
#include "castle/events/signal_ipc_config.h"
```

## Embedded notes

- Keep IPC limits explicit; increasing a capacity increases object storage.
