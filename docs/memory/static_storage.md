# Fixed object storage

## Overview

Provides an array of raw storage slots that can hold objects constructed and destroyed explicitly.

## Header

```cpp
#include "castle/memory/static_storage.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| `static_storage<T, N>` | Reserve space for `N` objects of `T`. |
| `address(index)` | Return the storage address for one slot. |
| `get_address<T>()` | Return a typed address for storage helpers where provided. |

## Example

```cpp
castle::memory::static_storage<T, 8> storage;
castle::memory::construct_at<T>(storage.address(0), 123);
```

## Embedded notes

- The storage itself does not create live objects.
- This is the core mechanism behind Castle’s heap-free containers.
