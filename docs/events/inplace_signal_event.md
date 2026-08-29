# `castle/events/inplace_signal_event.h` — POSIX signal dispatcher (SBO)

**Header:** `castle/events/inplace_signal_event.h`
**Namespace:** `castle::events`
**Sample:** [`samples/sample_inplace_signal_event.cpp`](../../samples/sample_inplace_signal_event.cpp)

## Purpose

Owning twin of `signal_event`. Each subscribed callable is stored
**by value** in an `inplace_callback_registry` slot (fixed-size inline
buffer, no heap), so callables with captured state can be registered
directly.

## Design notes

- Uses the same `signal_config<Signal, MaxCallback, StorageSize,
  StorageAlignment>` pack; all fields (including storage) are honoured.
- Same **immediate dispatch** model as `signal_event` — callbacks run
  in signal context; the async-signal-safety constraints apply here
  too (see the sibling doc).
- Same atomic `install()` / `uninstall()` ready-gate protocol.
- Error type: `event_subscription_error { ok, full, invalid_callback,
  invalid_subscription, signal_disabled, system_call_error }`.

## ⚠ Async-signal-safety

Every warning from `signal_event.md` applies. Note in particular:
constructing a **new** callback from signal context is impossible — do
all registration/unregistration from normal thread context, and keep
the callback body itself minimal (atomic flag, self-pipe write, …).

## API (all static)

| Method                                                | Notes                              |
| ----------------------------------------------------- | ---------------------------------- |
| `register_callback<Signal>(Callable&&, err_out)`      | Owning subscribe (SBO)             |
| `install()` / `uninstall()`                           | Arm / disarm `sigaction`           |
| `enable_signal<Signal>()` / `disable_signal<Signal>()`| Bitset gate                        |
| `is_signal_enabled<Signal>()`                         | `bool`                             |
| `clear<Signal>()` / `clear_all()`                     | Invalidate outstanding handles     |

## Example

```cpp
using castle::events::signal;
using signals_t = castle::events::inplace_signal_event<
    castle::events::signal_config<signal::sigint,  4>,
    castle::events::signal_config<signal::sigterm, 1>
>;

std::atomic_flag stop_requested = ATOMIC_FLAG_INIT;
auto sub = signals_t::register_callback<signal::sigint>(
    [&] { stop_requested.test_and_set(); });   // capture-by-ref lambda OK

signals_t::install();
while (!stop_requested.test()) tick();
signals_t::uninstall();
```

## See also

- `signal_event.h`, `signal_config.h`.
- `castle/callbacks/inplace_callback_registry.h` — storage backend.
