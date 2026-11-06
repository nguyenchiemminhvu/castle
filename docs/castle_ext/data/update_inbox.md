# UpdateInbox

## Overview

`castle_ext::data::update_inbox<Ts...>` is the receiving end of data-layer notifications for one consumer thread, such as a service or an application adapter. It implements `castle::design_patterns::observer<update_set<Ts...>>`, so it can be attached to a [`data_pool`](data_pool.md) or to any `castle::design_patterns::observable` that forwards update sets.

`notify()` can be called from producer threads. It merges the set into atomic words and, when the inbox goes from idle to pending, calls an optional wake hook such as a semaphore release. The consumer drains all pending types at once with `take()` and then reads only those types from the pool.

## Header

`#include "castle_ext/data/update_inbox.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../include/castle/core/types.hpp)
- [`castle/callbacks/function.hpp`](../../callbacks/function.md)
- [`castle/design_patterns/observer.hpp`](../../design_patterns/observer.md)
- [`castle/sync/atomic.hpp`](../../sync/atomic.md)
- [`castle_ext/data/update_set.hpp`](update_set.md)

## Public API

| API | Description |
|---|---|
| `update_inbox<Ts...>` | Inbox for `update_set<Ts...>`; `final`, derives from `observer<update_set<Ts...>>`. |
| `set_type` | `update_set<Ts...>`. |
| `wake_callback` | `castle::callbacks::function<void()>`. |
| `update_inbox()` | Idle inbox with no wake hook; the consumer polls `has_pending()`. |
| `explicit update_inbox(wake_callback wake)` | Idle inbox that calls `wake` whenever it goes from idle to pending. |
| `set_wake(wake_callback wake)` | Binds or replaces the wake hook after construction. Not synchronized; call before the inbox is attached to a pool or observable. |
| `notify(const set_type& updates)` | Adds `updates` to the pending set. Lock-free; safe from any thread. Overrides `observer::notify`. |
| `take()` | Removes and returns every pending type. The result is empty when nothing is pending. |
| `has_pending()` | Returns whether at least one type is pending. Safe from any thread. |

## Usage Example

See [`samples/castle_ext/data/update_inbox.cpp`](../../../samples/castle_ext/data/update_inbox.cpp).

```cpp
castle::semaphore<1> wake_up(0U);
castle_ext::data::update_inbox<imu_sample, gnss_fix> inbox{[&wake_up]() noexcept
{
    (void)wake_up.release();
}};
pool.add_observer(&inbox);          // or service_observable.add_observer(&inbox)

// Consumer thread
for (;;)
{
    wake_up.acquire();
    const auto changed = inbox.take();
    if (changed.contains<imu_sample>())
    {
        process(pool.get_snapshot<imu_sample>());
    }
}
```

## Constraints & Notes

- Notifications are coalesced. If a type is notified several times before `take()`, it is reported once. The consumer then reads the latest value from the pool, so no update is lost; only intermediate values are skipped. For event streams where every sample matters, use a queue such as `castle::container::mirrored_ring_buffer` instead.
- `notify()` does one atomic OR per non-empty 32-bit storage word, with no locks and no allocation.
- The wake hook runs in the notifier's context, which is often a HAL thread. Keep it short and thread-safe, e.g. a semaphore release or an event-flag set. It fires when a storage word goes from empty to non-empty, so a consumer that waits on it never misses pending updates. With more than 32 types, one notification can wake the consumer more than once; `take()` then returns an empty set.
- `take()` belongs to the single consumer thread that owns the inbox. Several consumers that share one inbox would take updates away from each other; give each consumer its own inbox.
- Set the wake hook at construction or with `set_wake()` before the inbox is attached. Do not destroy the inbox while it is still attached to an observable.
