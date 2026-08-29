# `castle/callbacks/inplace_callback_registry.h` — Owning pub/sub, no heap

**Header:** `castle/callbacks/inplace_callback_registry.h`
**Namespace:** `castle::callbacks`
**Sample:** [`samples/sample_inplace_callback_registry.cpp`](../../samples/sample_inplace_callback_registry.cpp)

## Purpose

An **owning** twin of `callback_registry`. Callbacks are stored **by
value** in an `inplace_function`-backed slot, so stateful lambdas (with
captures), functors, and member closures can be subscribed **without
any external lifetime management** and **without heap allocation**.

## Design notes

- Each slot holds an `inplace_function<Signature, StorageSize,
  StorageAlignment>` plus a `uint32_t generation` for identity-safe
  unsubscribe.
- Subscription handle is `callback_subscription`, mirroring
  the design in `callback_subscription` (back-pointer via
  the type-erased `i_unsubscribable` base, generation-checked
  unsubscribe, copyable value type with first-wins semantics).
- **`void` return only** — same rationale as `callback_registry`.
- **No heap, no exceptions.** Callables that don't fit in the inline
  buffer fail to compile (`static_assert` from `inplace_function`).
- **Non-copyable, non-movable** — outstanding subscriptions embed a
  back-pointer to `this`.

## Error codes

```cpp
enum class callback_subscription_error : uint8_t {
    ok = 0,
    full,
    invalid_callback,
    invalid_subscription
};
```

## API (excerpt)

| Method                                    | Notes                                   |
| ----------------------------------------- | --------------------------------------- |
| `subscribe(Callable&&, err_out = nullptr)`| Copies/moves into an internal slot      |
| `unsubscribe_slot(idx, gen)`              | Type-erased entry                       |
| `invoke(args...)` / `operator()(args...)` | Fans out in registration order          |
| `clear()`                                 | Bumps generation on all slots           |
| `size()` / `capacity()` / `full()`        | Introspection                           |

## Example

```cpp
#include <castle/callbacks/inplace_callback_registry.h>
using namespace castle::callbacks;

inplace_callback_registry<4, void(int)> hub;

int captured = 10;
auto sub = hub.subscribe([captured](int v) {
    std::printf("%d\n", v + captured);   // 15 for v=5
});

hub.invoke(5);
sub.unsubscribe();
```

## Choosing between the two registries

| Property                     | `callback_registry`         | `inplace_callback_registry` |
| ---------------------------- | --------------------------- | ---------------------------- |
| Owns callables?              | No (non-owning pointers)    | Yes (SBO storage)            |
| Supports lambdas w/ captures | Only via external storage   | Yes, natively                |
| Per-callback footprint       | Zero (with `function_ct*`)  | `StorageSize` bytes / slot   |
| Heap usage                   | None                        | None                         |

## See also

- `castle/callbacks/inplace_function.h` — storage backend.
- `castle/callbacks/callback_registry.h` — non-owning counterpart.
- `castle/events/inplace_event_dispatcher.h` — multi-event façade.
