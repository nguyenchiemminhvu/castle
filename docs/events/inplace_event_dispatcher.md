# `castle/events/inplace_event_dispatcher.h` — Tag-keyed dispatcher (SBO)

**Header:** `castle/events/inplace_event_dispatcher.h`
**Namespace:** `castle::events`
**Sample:** [`samples/sample_inplace_event_dispatcher.cpp`](../../samples/sample_inplace_event_dispatcher.cpp)

## Purpose

Owning twin of `event_dispatcher`. Each subscribed callable is stored
**by value** in a fixed inline buffer via `inplace_function` /
`inplace_callback_registry`, so stateful lambdas (with captures) work
out of the box **without heap allocation** and without external
lifetime management.

## Design notes

- Same `event_config<...>` pack as `event_dispatcher`; here **all**
  fields are honoured (including `StorageSize` and `StorageAlignment`).
- Signatures are required to be `void(Args...)`; enforced by the same
  `static_assert` chain.
- Non-copyable / non-movable — subscription handles carry a
  back-pointer to the concrete per-tag registry.
- Error type: `event_subscription_error { ok, full,
  invalid_callback, invalid_subscription, event_disabled }`.

## API (excerpt)

| Method                                              | Notes                                     |
| --------------------------------------------------- | ----------------------------------------- |
| `register_callback<Tag>(Callable&&, err = nullptr)` | Returns `callback_subscription`   |
| `dispatch_event<Tag>(args...)`                      | Fan-out in registration order             |
| `enable_event<Tag>()` / `disable_event<Tag>()`      | Bitset gate                               |
| `is_event_enabled<Tag>()`                           | `bool`                                    |
| `clear<Tag>()` / `clear_all()`                      | Invalidate outstanding subscriptions      |
| `size<Tag>()` / `capacity<Tag>()`                   | Introspection                             |

## Example

```cpp
struct TimerExpired {};

using dispatcher_t = castle::events::inplace_event_dispatcher<
    castle::events::event_config<TimerExpired, 8, void(std::uint32_t, std::uint32_t)>
>;

dispatcher_t d;
int scale = 4;
auto sub = d.register_callback<TimerExpired>(
    [scale](std::uint32_t id, std::uint32_t ts) {
        do_work(id, ts * scale);
    });
d.dispatch_event<TimerExpired>(42u, 1000u);
sub.unsubscribe();
```

## When to choose it over `event_dispatcher`

| Need                                | Prefer                          |
| ----------------------------------- | ------------------------------- |
| Zero-storage `function_ct_*`        | `event_dispatcher`              |
| Lambdas with captures               | `inplace_event_dispatcher`      |
| Minimum per-slot footprint          | `event_dispatcher`              |
| No external callback lifetime mgmt  | `inplace_event_dispatcher`      |

## See also

- `event_dispatcher.h`, `event_config.h`.
- `castle/callbacks/inplace_callback_registry.h`,
  `castle/callbacks/inplace_function.h` — storage layer.
