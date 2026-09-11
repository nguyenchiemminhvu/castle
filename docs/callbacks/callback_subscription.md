# Callback subscription handle

## Overview

A small handle returned by `callback_registry` and `inplace_callback_registry`. It lets application code remove a callback without knowing the registry template type.

## Header

```cpp
#include "castle/callbacks/callback_subscription.h"
```

**Namespace:** `castle::callbacks`

## Main API

| API | Purpose |
|---|---|
| `callback_subscription` | Stores owner, slot index, generation, and validity state. |
| `valid()` / `operator bool` | Check whether the handle can unsubscribe a live slot. |
| `unsubscribe()` | Remove the registered callback and invalidate the handle. |
| `reset()` | Invalidate the handle without touching a registry slot. |
| `index()` / `generation()` | Inspect the stored subscription identity. |

## Example

```cpp
auto sub = registry.subscribe(callback);
if (sub) {
    sub.unsubscribe();
}
```

## Embedded notes

- Copying a subscription copies the same identity; the first successful unsubscribe wins.
- Generation numbers protect against stale-handle bugs.
