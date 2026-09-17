# `castle/events/inplace_tick_timer.h` — Tick-driven software timer (SBO)

**Header:** `castle/events/inplace_tick_timer.h`
**Namespace:** `castle::events`
**Sample:** [`samples/sample_inplace_tick_timer.cpp`](../../samples/sample_inplace_tick_timer.cpp)

## Purpose

Owning twin of `tick_timer<MaxCallback>`. Each subscribed callable is
stored **by value** in an `inplace_callback_registry` slot — no heap
allocation and no external lifetime management, so lambdas with
captures are the natural way to register timeouts.

Semantics are otherwise identical to `tick_timer`: passive
accumulator, driven by `on_tick()`, supports `one_shot`, `periodic`,
and `n_repeat` modes.

## API (excerpt)

| Method                                          | Notes                                     |
| ----------------------------------------------- | ----------------------------------------- |
| `set_period(tick_type)`                         | `invalid_config` for `0`                  |
| `register_callback(Callable&&, err_out)`        | Returns `callback_subscription`   |
| `start(mode)` / `start(mode, N)`                | `not_configured` if period is unset       |
| `stop()`                                        | Halts and rewinds accumulator             |
| `on_tick(elapsed_ticks = 1)`                    | Advances the timer                        |
| `remaining() const` / `is_running() const`      | Introspection                             |

Error type: `event_subscription_error { ok, full, invalid_callback,
invalid_subscription, invalid_config, not_configured }`.

## Example

```cpp
#include <castle/events/inplace_tick_timer.h>
using castle::events::inplace_tick_timer;
using castle::events::tick_timer_mode;

inplace_tick_timer<4> t;
t.set_period(50);

int fire_count = 0;
auto sub = t.register_callback([&] { ++fire_count; });
t.start(tick_timer_mode::n_repeat, 3);          // fire 3 times

for (int i = 0; i < 200; ++i) t.on_tick(1);     // fire_count == 3
```

## See also

- `tick_timer.h` — non-owning counterpart.
- `castle/callbacks/inplace_callback_registry.h` — storage backend.
- `castle/callbacks/callback_policy.h` — poll-based throttling
  alternatives (`throttle`, `every_n`).
