# Execution Policy

## Overview
`exec_policy.hpp` provides small callback wrappers that decide **when** a stored callback runs: once, every N calls, on change, inside an armed time window, throttled, or periodically. Use it when callback timing/gating should stay explicit, allocation-free, and poll-driven.

## Header
`#include "castle/callbacks/exec_policy.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`../core/types.md`](../core/types.md)
- [`../core/traits.md`](../core/traits.md)
- [`../atomic/atomic.md`](../atomic/atomic.md)
- [`../sync/mutex.md`](../sync/mutex.md)
- [`../sync/scoped_mutex.md`](../sync/scoped_mutex.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/optional.md`](../utility/optional.md)
- [`../chrono/chrono.md`](../chrono/chrono.md)

## Public API
| API | Description |
| --- | --- |
| `policy::single_thread` / `policy::concurrent` | Concurrency backends used by every policy. `single_thread` uses plain values; `concurrent` uses Castle atomics and short mutex-protected sections where needed. |
| `policy::once<Callback, Mode>` / `make_once()` | Fires once until `reset()`. |
| `policy::armed_window<Callback, Mode, Clock>` / `make_armed_window()` | Fires at most once before a deadline, then latches to `fired` or `expired`. |
| `policy::every_n<Callback, Mode>` / `make_every_n()` / `make_every_n_ct()` | Fires on every Nth call. `n == 0` is normalized to `1`. |
| `policy::on_change<T, Callback, Mode>` / `make_on_change()` | Fires when the observed value differs from the stored previous value. |
| `policy::throttle<Callback, Mode, Clock>` / `make_throttle()` | Fires only if the configured interval elapsed since the previous fire. Extra calls are dropped. |
| `policy::periodic<Callback, Mode, Clock>` / `make_periodic()` | Poll-driven schedule that fires once per elapsed period and preserves phase without drift. |

## Usage Example
See `samples/sample_exec_policy.cpp`.

```cpp
using namespace castle::chrono::literals::chrono_literals;

int hits = 0;
auto once = castle::callbacks::policy::make_once([&hits]() noexcept { ++hits; });
once.execute();

auto every = castle::callbacks::policy::make_every_n(4U, [&hits]() noexcept { ++hits; });
every();
```

## Constraints & Notes
- No heap allocation.
- Time-based policies are poll-driven; they do not create threads or timers.
- The callback always runs in the caller's thread.
- `single_thread` provides no synchronization and assumes external serialization.
- `concurrent` only synchronizes the state each policy actually protects; observer/reset methods that read or write plain members still require external coordination where documented in the header.
- `on_change` stores its last value in `castle::optional`, and `throttle` tracks its last fire time the same way.
