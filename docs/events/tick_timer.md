# tick_timer

## Overview
`castle::events::tick_timer` is a passive software timer driven by externally supplied ticks. It exists for deterministic embedded timing where the application, ISR, or scheduler already owns the time base and callback storage must remain fixed and heap-free.

## Header
`#include "castle/events/tick_timer.hpp"`

## Dependencies
- [`function`](../callbacks/function.md)
- [`function_registry`](../callbacks/function_registry.md)
- [`subscription`](../callbacks/subscription.md)
- [`status`](../error/status.md)

## Public API
| API | Description |
| --- | --- |
| `enum class tick_timer_mode : uint8_t` | Expiration modes: `one_shot`, `periodic`, and `n_repeat`. |
| `template <size_type MaxCallback, size_type CallbackStorageSize, size_type CallbackStorageAlignment> class tick_timer` | Timer storing up to `MaxCallback` timeout callbacks by value. |
| `using tick_type = uint32_t` | Tick unit used for periods, elapsed time, and repeat counts. |
| `using error = castle::status` | Error type returned by configuration and control APIs. |
| `using mode = tick_timer_mode` | Convenience alias for the timer mode enum. |
| `using subscription = callbacks::subscription` | Callback handle returned by `register_callback()`. |
| `using registry_type = callbacks::function_registry<...>` | Underlying callback registry type. |
| `error set_period(tick_type)` | Stores a non-zero period in ticks. Returns `status::invalid_config` when the period is zero. |
| `template <typename Callback> subscription register_callback(Callback&&, error* = nullptr)` | Subscribes a `void()` callback by value. |
| `error start(mode run_mode = mode::periodic, tick_type repeat_count = 0)` | Starts or restarts the timer, resets the accumulated counter, and configures the mode. |
| `void stop()` | Stops the timer and clears the accumulated counter plus repeat budget. |
| `void pause()` | Stops ticking without clearing the accumulated counter. |
| `error resume()` | Sets the running flag again when a non-zero period is configured. |
| `void reset()` | Clears the accumulated counter without changing mode or running state. |
| `void on_tick(tick_type elapsed_ticks = 1)` | Advances the timer and invokes callbacks once per completed period. |
| `bool is_running() const` | Returns the current running flag. |
| `tick_type period() const` / `tick_type elapsed() const` / `tick_type remaining() const` | Read the configured period, accumulated ticks, and remaining ticks until the next expiry. |
| `mode current_mode() const` | Returns the stored mode. |
| `tick_type repeats_remaining() const` | Returns the remaining repeat budget for `mode::n_repeat`, otherwise zero. |
| `size_type callback_count() const` | Returns the number of active callbacks. |
| `static constexpr size_type callback_capacity()` | Returns `MaxCallback`. |
| `static constexpr tick_type max_period()` | Returns `numeric_limits<tick_type>::max()`. |
| `void clear_callbacks()` | Removes all registered callbacks without changing timer state. |
| `registry_type& registry()` / `const registry_type& registry() const` | Exposes the underlying callback registry for advanced integration. |

## Usage Example
See [`samples/sample_tick_timer.cpp`](../../samples/sample_tick_timer.cpp).

```cpp
castle::events::tick_timer<2U> timer;
std::uint32_t fired = 0U;

timer.set_period(3U);
timer.register_callback([&fired]() { ++fired; });
timer.start(castle::events::tick_timer_mode::periodic);

timer.on_tick(6U); // fires twice
timer.stop();
```

## Constraints & Notes
- The timer owns callbacks inline through `function_registry`; Castle performs no heap allocation.
- The timer never creates a thread and never reads a hardware or OS clock. Progress occurs only when `on_tick()` is called.
- Callback invocation order matches callback subscription order.
- `on_tick()` performs catch-up: if one call spans multiple periods, periodic timers fire multiple times in that call.
- `pause()` preserves the accumulated counter, while `stop()` clears both the counter and the repeat budget.
- `resume()` only checks that a non-zero period exists. It does not restore a previous repeat budget cleared by `stop()` or by a completed `n_repeat` run.
- The type has no internal synchronization; coordinate `on_tick()`, callback registration, and registry access externally if different contexts touch the same timer.
