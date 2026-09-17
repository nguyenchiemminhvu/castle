# Fixed-capacity callback registry

## Overview

A non-owning registry for a fixed number of callbacks. It stores pointers to existing `delegate_base` objects and returns lightweight subscription handles.

## Header

```cpp
#include "castle/callbacks/delegate_registry.h"
```

**Namespace:** `castle::callbacks`

## Main API

| API | Purpose |
|---|---|
| `delegate_registry<max, void(Args...)>` | Fixed-capacity registry. Callback return type must be `void`. |
| `subscribe()` | Add a callback and receive a `subscription`. |
| `invoke()` | Invoke active callbacks in subscription order. |
| `unsubscribe_slot()` | Internal type-erased entry used by a subscription. |
| `clear()` / `size()` style queries | Manage active registrations and inspect capacity where provided. |

## Example

```cpp
castle::callbacks::delegate_ptr<void(int)> cb(&handler);
castle::callbacks::delegate_registry<4, void(int)> r;
auto sub = r.subscribe(&cb);
r.invoke(42);
sub.unsubscribe();
```

## Embedded notes

- The registry does not own the callback object.
- A callback must outlive its subscription.
- Slot generations prevent a stale subscription from removing a reused slot.
