# DiagnosticManager

## Overview

`castle::diag::diagnostic_manager` is a polled, compile-time-configured diagnostic evaluator for embedded applications. Components report current fault conditions to a central manager. The manager qualifies each condition immediately, after consecutive update cycles, or after a steady-clock duration. It can also combine already-qualified diagnostics into composite faults, pause selected monitors, and invoke a recovery callback once when a fault becomes active.

The component intentionally does not create a background task or timer. The application chooses the diagnostic cadence and calls `update()` from its main loop, scheduler, or health-monitor task.

## Header

```cpp
#include "castle_ext/diag/diagnostic_manager.hpp"
```

## Dependencies

- [`castle/chrono/clocks.hpp`](../../../include/castle/chrono/clocks.hpp) — default `steady_clock` and time-point support.
- [`castle/core/compiler.hpp`](../../../include/castle/core/compiler.hpp) — portable language and `noexcept` macros.
- [`castle/core/traits.hpp`](../../../include/castle/core/traits.hpp) — compile-time callable and type checks.
- [`castle/core/types.hpp`](../../../include/castle/core/types.hpp) — Castle fixed-width size types.
- [`castle/error/status.hpp`](../../../include/castle/error/status.hpp) — deterministic API status results.
- [`castle/utility/tuple.hpp`](../../../include/castle/utility/tuple.hpp) — fixed-size heterogeneous state storage.

The uploaded simplified core snapshot referenced `castle/math/{gcd,lcm,ratio}.hpp` from its chrono headers but did not contain those files. Minimal compile-time implementations are included alongside this component so the existing Castle chrono dependency can be used as intended.

## Public API

| Type / method | Description |
|---|---|
| `immediate` | Qualifies a condition on its first true update. |
| `after_cycles<N>` | Requires `N` consecutive true update evaluations. The first true evaluation counts as cycle one. |
| `after_duration<Duration, Count>` | Requires the condition to stay true for `Count` units of the specified Castle chrono duration. |
| `rearm_on_clear` | Re-arms the recovery action automatically after the fault clears. |
| `one_shot` | Leaves the recovery action disarmed after the first activation until `rearm<Tag>()` is called. |
| `diagnostic_event<Tag, Qualification, Rearm>` | Declares a source event whose current condition is supplied through `report<Tag>()`. |
| `active<Tag>` | Expression that tests whether a diagnostic is qualified and active. |
| `all_of<...>`, `any_of<...>`, `not_<...>` | Boolean expressions for constructing composite diagnostics. |
| `diagnostic_group<Tag, Expression, Qualification, Rearm>` | Declares a composite diagnostic based on previously declared event or group tags. |
| `diagnostic_manager<Rules...>` | Default-constructible manager owning all per-rule state. Copying is disabled; moving is supported. Tags must be unique; group dependencies must be declared first. |
| `diagnostic_manager::rule_count` | Compile-time number of configured event and group rules. |
| `diagnostic_manager::time_point`, `duration` | Aliases for the Castle steady-clock time point and duration types. |
| `diagnostic_manager::recovery_callback` | `noexcept` function-pointer type `void (*)(void*)` accepted by the context-based recovery overload. |
| `report<Tag>(bool)` | Stores the current raw condition for a source event. Multiple reports before `update()` coalesce to the last value. |
| `set_recovery<Tag>(callable)` | Installs a non-owning, `noexcept` nullary function object. The lvalue object must outlive the registration. |
| `set_recovery<Tag>(callback, context)` | Installs an embedded-style `void (*)(void*) noexcept` callback with a user-owned context pointer. |
| `clear_recovery<Tag>()` | Removes a recovery callback without changing diagnostic state. |
| `pause<Tag>()`, `resume<Tag>()` | Pause or resume one monitor. Existing confirmed state is held while paused; qualification progress is discarded. |
| `pause()`, `resume()` | Pause or resume the entire manager. Raw reports are still accepted while globally paused. |
| `rearm<Tag>()` | Re-arm a `one_shot` recovery action. Returns `status::invalid_argument` while that diagnostic is active. |
| `is_active<Tag>()`, `is_pending<Tag>()`, `is_enabled<Tag>()` | Query the qualified state, qualification progress, and effective enabled state. |
| `get_state<Tag>()` | Returns a `diagnostic_snapshot` containing raw/qualified state and counters. |
| `update()` | Evaluate rules using `castle::chrono::steady_clock::now()`. |
| `update(time_point)` | Evaluate rules at an explicit steady-clock time point; useful for deterministic tests. Returns `update_result`. |
| `update_result` | Counts newly activated diagnostics, cleared diagnostics, and recovery callbacks invoked during one update. |
| `diagnostic_snapshot` | Read-only state snapshot, including qualification progress and transition counters. |

## Usage Example

```cpp
#include "castle_ext/diag/diagnostic_manager.hpp"

struct can_timeout {};
struct low_voltage {};
struct power_failure {};

using diagnostics = castle::diag::diagnostic_manager<
    castle::diag::diagnostic_event<can_timeout>,
    castle::diag::diagnostic_event<low_voltage>,
    castle::diag::diagnostic_group<
        power_failure,
        castle::diag::all_of<
            castle::diag::active<can_timeout>,
            castle::diag::active<low_voltage>>>>;

int main()
{
    diagnostics diag;
    unsigned restart_requests = 0U;
    auto restart_node = [&restart_requests]() noexcept { ++restart_requests; };
    diag.set_recovery<power_failure>(restart_node);

    diag.report<can_timeout>(true);
    diag.report<low_voltage>(true);
    const auto result = diag.update();

    return (diag.is_active<power_failure>() &&
            result.recoveries_executed == 1U && restart_requests == 1U) ? 0 : 1;
}
```

A complete standalone sample that exercises groups, pause/resume, recovery re-arming, snapshots, and explicit timestamps is available at [`samples/castle_ext/diag/diagnostic_manager.cpp`](../../../samples/castle_ext/diag/diagnostic_manager.cpp).

## Constraints & Notes

- Rule count and rule types are fixed at compile time. Storage is one fixed state record per configured rule; there is no registration container or runtime growth.
- No heap allocation, exceptions, RTTI, or virtual functions are used by this component.
- The manager is polled. `after_cycles<N>` counts calls to `update()` while a condition remains true, not calls to `report()`.
- `after_duration` uses `castle::chrono::steady_clock`; explicit timestamps passed to `update(time_point)` must come from the same monotonic time base.
- Source event reports are level-based. The last reported boolean is retained until another report arrives.
- Groups evaluate qualified diagnostic state, not raw source values. All source events are evaluated first, followed by groups in declaration order. A group may depend only on rules declared earlier; this is checked at compile time.
- Recovery callbacks are non-owning. A function object is stored through a `noexcept` thunk plus context pointer; the lvalue object must outlive the binding. Alternatively, pass a `void (*)(void*) noexcept` callback and explicit context pointer. The caller owns the context lifetime. `clear_recovery<Tag>()` removes the binding.
- A recovery action is invoked only on an inactive-to-active transition and is disarmed for that failure episode. With `rearm_on_clear`, a normal condition automatically re-arms it. With `one_shot`, call `rearm<Tag>()` after the diagnostic has cleared.
- Pausing holds a confirmed active/inactive state and discards pending qualification. On resume, qualification starts afresh using the latest reported source values. Dependent groups therefore see the held state while a source monitor is paused.
- The manager is not internally synchronized. Serialize `report`, `update`, pause/resume, and callback configuration from the application.
- Runtime is deterministic and bounded by the configured rule count and expression size. Each update is O(number of rules + total expression nodes); boolean expressions short-circuit.
