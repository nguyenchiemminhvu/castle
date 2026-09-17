# Subscription

## Overview
`subscription.hpp` provides the small handle returned by callback registries and the type-erased owner interface used to unsubscribe a slot later. Use it when callback removal must be possible without exposing the concrete registry template type.

## Header
`#include "castle/callbacks/subscription.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`../error/status.md`](../error/status.md)

## Public API
| API | Description |
| --- | --- |
| `i_unsubscribable` | Type-erased interface implemented by registries through `unsubscribe_slot(index, generation)`. |
| `subscription()` | Constructs an invalid handle. |
| `subscription(owner, index, generation)` | Constructs a handle that refers to one registry slot. |
| `valid()` / `operator bool()` | Returns whether the handle is locally marked valid and still has a non-null owner pointer. |
| `index()` / `generation()` | Exposes the stored slot identity for diagnostics or tests. |
| `unsubscribe()` | Calls the owner interface, then resets the handle regardless of success. |
| `reset()` | Invalidates the handle without touching the registry. |

## Usage Example
See `samples/sample_subscription.cpp`.

```cpp
castle::callbacks::subscription sub{};
if (!sub.valid())
{
    sub.reset();
}
```

## Constraints & Notes
- No heap allocation.
- `subscription` is a value type; copying duplicates the same slot identity.
- Only the first successful `unsubscribe()` for a given slot identity can succeed; stale copies return `status::invalid_subscription`.
- Destroying a `subscription` object does **not** unsubscribe automatically.
- A `subscription` that outlives its registry must not call `unsubscribe()` because the stored owner pointer dangles.
- The handle itself provides no synchronization.
