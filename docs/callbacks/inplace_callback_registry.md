# Owning fixed-capacity callback registry

## Overview

A fixed-capacity callback registry that stores each callback by value in `inplace_function`. Captured lambdas are supported without external lifetime management.

## Header

```cpp
#include "castle/callbacks/inplace_callback_registry.h"
```

**Namespace:** `castle::callbacks`

## Main API

| API | Purpose |
|---|---|
| `inplace_callback_registry<...>` | Fixed number of slots plus per-slot in-place callable storage. |
| `subscribe(callback)` | Register a ready-made `inplace_function` or any compatible callable. |
| `invoke()` | Invoke active callbacks in registration order. |
| `callback_subscription` | Returned handle for self-unsubscribe. |
| `clear()` / active count | Remove callbacks and inspect the number of active slots. |

## Example

```cpp
castle::callbacks::inplace_callback_registry<4, void(int)> r;
auto sub = r.subscribe([](int v) { /* ... */ });
r.invoke(42);
sub.unsubscribe();
```

## Embedded notes

- Unlike `callback_registry`, this registry owns the callable storage.
- Capacity and callable storage are fixed at compile time.
- Return type is intentionally restricted to `void`.
