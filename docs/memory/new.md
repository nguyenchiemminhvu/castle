# Placement construction support

## Overview

Minimal placement-new support used by in-place callable and object storage code.

## Header

```cpp
#include "castle/memory/new.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| Placement construction | Create a `T` directly in caller-provided storage. |
| Raw allocation boundary | No heap allocator is introduced by this header. |

## Example

```cpp
// Used internally by inplace_function and storage types.
```

## Embedded notes

- This is not a replacement for `new` as a heap allocator.
