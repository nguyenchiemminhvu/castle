# Scoped mutex lock

## Overview

RAII helper that locks a Castle mutex at construction and unlocks it at scope exit.

## Header

```cpp
#include "castle/mutex/scoped_mutex.h"
```

**Namespace:** `castle::mutex`

## Main API

| API | Purpose |
|---|---|
| `scoped_mutex` | Lock guard for a Castle mutex. |
| Constructor/destructor | Acquire and release the mutex automatically. |

## Example

```cpp
castle::mutex::scoped_mutex guard(mutex);
// protected work
```

## Embedded notes

- RAII is local and deterministic here: there is no allocation.
- Use short critical sections, especially around interrupt/shared-state code.
