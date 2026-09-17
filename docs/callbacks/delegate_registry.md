# Delegate Registry

## Overview
`delegate_registry.hpp` provides a fixed-capacity, non-owning callback list for delegate objects. Use it when callbacks already exist elsewhere and you only need deterministic subscribe/invoke/unsubscribe behavior.

## Header
`#include "castle/callbacks/delegate_registry.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`../error/status.md`](../error/status.md)
- [`../utility/move.md`](../utility/move.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`delegate.md`](delegate.md)
- [`subscription.md`](subscription.md)
- [`../container/array.md`](../container/array.md)

## Public API
| API | Description |
| --- | --- |
| `delegate_registry<Max, void(Args...)>` | Fixed-capacity registry of borrowed `delegate_base<void(Args...)>*` pointers. |
| `subscribe(callback, out_error)` | Returns a `subscription`; reports `status::ok`, `status::invalid_callback`, or `status::full`. Complexity: O(capacity). |
| `unsubscribe_slot(index, generation)` | Type-erased removal entry used by `subscription`. |
| `invoke(args...)` / `operator()(args...)` | Invokes active callbacks in increasing slot order. Complexity: O(capacity). |
| `clear()` | Removes all active callbacks and invalidates existing subscriptions. Complexity: O(capacity). |
| `size()` / `empty()` / `capacity()` | Inspect active count and compile-time capacity. |

## Usage Example
See `samples/sample_delegate_registry.cpp`.

```cpp
void on_value(int) noexcept {}

castle::callbacks::delegate_ptr<void(int)> callback{&on_value};
castle::callbacks::delegate_registry<4U, void(int)> registry;
castle::callbacks::subscription sub = registry.subscribe(&callback);
registry(1);
sub.unsubscribe();
```

## Constraints & Notes
- No heap allocation.
- Stores non-owning callback pointers; each delegate object must outlive the active registration.
- Capacity is fixed at compile time.
- Only `void(Args...)` callback signatures are supported.
- `subscription` destruction does not unsubscribe.
- A `subscription` that outlives the registry must not call `unsubscribe()` because its owner pointer dangles.
- No internal synchronization; concurrent access requires external locking/serialization.
