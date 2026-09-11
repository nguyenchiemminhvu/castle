# Address-of helper

## Overview

Obtains an object address without being affected by a user-defined `operator&`.

## Header

```cpp
#include "castle/memory/addressof.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| `addressof(obj)` | Return the real address of an object. |

## Example

```cpp
Widget w;
Widget* p = castle::memory::addressof(w);
```

## Embedded notes

- Useful in low-level container and storage code.
