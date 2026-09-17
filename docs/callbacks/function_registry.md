# Function Registry

## Overview
`function_registry.hpp` provides a fixed-capacity callback list that owns each registered callable in inline storage. Use it when you need registry-managed lifetimes for stateful lambdas or other callables without heap allocation.

## Header
`#include "castle/callbacks/function_registry.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/config.md`](../core/config.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`../error/status.md`](../error/status.md)
- [`../utility/move.md`](../utility/move.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../container/array.md`](../container/array.md)
- [`function.md`](function.md)
- [`subscription.md`](subscription.md)

## Public API
| API | Description |
| --- | --- |
| `function_registry<Max, void(Args...), StorageSize, StorageAlignment>` | Fixed-capacity registry that owns one `function<>` per active slot. |
| `subscribe(function&&, out_error)` | Moves a ready-made `function` into the first free slot. Complexity: O(capacity). |
| `subscribe(callable, out_error)` | Wraps any compatible callable in `function<>` and stores it. Complexity: O(capacity). |
| `unsubscribe_slot(index, generation)` | Type-erased removal entry used by `subscription`. |
| `invoke(args...)` / `operator()(args...)` | Invokes active callbacks in increasing slot order. Complexity: O(capacity). |
| `clear()` | Resets all active slots and invalidates existing subscriptions. Complexity: O(capacity). |
| `size()` / `empty()` / `capacity()` | Inspect active count and compile-time capacity. |

## Usage Example
See `samples/sample_function_registry.cpp`.

```cpp
castle::callbacks::function_registry<4U, void(int)> registry;
int total = 0;
auto sub = registry.subscribe([&total](int value) noexcept { total += value; });
registry.invoke(2);
sub.unsubscribe();
```

## Constraints & Notes
- No heap allocation.
- Capacity and per-slot callable storage are fixed at compile time.
- Only `void(Args...)` callback signatures are supported.
- Stored callables live inside the registry, so the original lambda/functor may go out of scope after successful subscription.
- `subscription` destruction does not unsubscribe.
- A `subscription` that outlives the registry must not call `unsubscribe()`.
- No internal synchronization; concurrent access requires external locking/serialization.
