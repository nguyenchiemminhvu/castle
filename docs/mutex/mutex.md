# Minimal mutex

## Overview

Small synchronization primitive for code that needs mutual exclusion without pulling in STL threading facilities.

## Header

```cpp
#include "castle/mutex/mutex.h"
```

**Namespace:** `castle::mutex`

## Main API

| API | Purpose |
|---|---|
| `mutex` | Lockable type with the Castle mutex API. |
| `lock()` / `unlock()` | Enter and leave the critical section. |
| Try-lock support | Use the non-blocking operation if provided by the target implementation. |

## Example

```cpp
castle::mutex::mutex m;
m.lock();
// protected access
m.unlock();
```

## Embedded notes

- The actual blocking mechanism depends on the target/compiler integration.
- Use `scoped_mutex` to make unlock-on-exit harder to forget.
