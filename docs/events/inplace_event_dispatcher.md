# In-place event dispatcher

## Overview

Event dispatcher variant that stores handler callables in Castle’s in-place callback facilities.

## Header

```cpp
#include "castle/events/inplace_event_dispatcher.h"
```

**Namespace:** `castle::events`

## Main API

| API | Purpose |
|---|---|
| Dispatcher type | Fixed-capacity event routing with in-object callback storage. |
| Registration/dispatch | Register and invoke handlers without external callback allocation. |

## Example

```cpp
// Suitable for captured lambdas that must stay on the stack/static object.
```

## Embedded notes

- Choose storage sizes deliberately because callable objects must fit their in-place buffers.
