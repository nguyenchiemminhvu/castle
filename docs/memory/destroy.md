# Object destruction helpers

## Overview

Destroys objects explicitly in raw/fixed storage.

## Header

```cpp
#include "castle/memory/destroy.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| `destroy_at(p)` | Call an object destructor at a supplied address. |
| `destroy_n(p, n)` | Destroy `n` consecutive live objects. |
| Destruction helpers | Support the manual lifetime management used by Castle containers. |

## Example

```cpp
castle::memory::destroy_at(p);
castle::memory::destroy_n(first, count);
```

## Embedded notes

- Only call destroy helpers for objects whose lifetime is active.
- This explicit lifetime model is important for fixed-capacity containers.
