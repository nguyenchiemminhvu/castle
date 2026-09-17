# Object-from-address helpers

## Overview

Turns a known object storage address into an object pointer using Castle’s low-level lifetime model.

## Header

```cpp
#include "castle/memory/object.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| `object_from_address<T>(p)` | Get `T*` from aligned storage that contains a live `T`. |
| Const forms | Preserve const qualification when requesting the pointer. |

## Example

```cpp
T* p = castle::memory::object_from_address<T>(storage.address(0));
```

## Embedded notes

- Only use the pointer after the object has been constructed in that storage.
