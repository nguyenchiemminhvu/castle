# Memory alignment

## Overview

Contains Castle’s low-level raw-storage, alignment, byte, and object-address helpers.

## Header

```cpp
#include "castle/memory/alignment.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| Aligned storage | `aligned_storage`/`aligned_storage_as_t` style type helpers. |
| Object address helpers | Convert raw storage addresses back to `T*` when a live object exists. |
| Storage/math helpers | Provide the building blocks used by `static_storage` and other containers. |

## Example

```cpp
using raw_t = castle::memory::aligned_storage_as_t<sizeof(T), T>;
```

## Embedded notes

- This header is for library-level storage work more than everyday application code.
