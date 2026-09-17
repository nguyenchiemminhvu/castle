# Raw aligned storage

## Overview

Generic raw storage types for objects that are constructed later.

## Header

```cpp
#include "castle/memory/storage.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| Storage type | Compile-time size/alignment raw storage. |
| Pointer access | Obtain byte/raw addresses suitable for placement construction. |

## Example

```cpp
castle::memory::storage<sizeof(T), alignof(T)> raw;
```

## Embedded notes

- Treat the returned memory as untyped bytes until an object is explicitly constructed.
