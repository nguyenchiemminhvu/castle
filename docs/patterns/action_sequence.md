# Action Sequence

## Overview
A fixed-capacity, self-contained sequence of one-shot calls and delays, modeled on the cocos2d-x `Sequence` (`CallFunc`, `DelayTime`). The sequence owns every step and every captured callable. Delays and elapsed times are `castle::chrono::duration` values. The owner supplies elapsed time through `update(elapsed)`; the sequence never reads a clock itself, so execution is deterministic and can be driven by a main loop, a `castle::events::tick_timer` callback, or differences of `castle::chrono::steady_clock::now()`.

## Header
`#include "castle/patterns/action_sequence.hpp"`

## Dependencies
- [`../callbacks/function.md`](../callbacks/function.md)
- [`../dsa/array.md`](../dsa/array.md)
- [`../chrono/duration.md`](../chrono/duration.md)
- [`../error/status.md`](../error/status.md)
- `castle/math/ratio.hpp`, `castle/utility/forward.hpp`
- `castle/core/compiler.hpp`, `config.hpp`, `traits.hpp`, `type_ranges.hpp`, `types.hpp`

## Public API
| API | Description |
| --- | --- |
| `action_sequence_state` | `idle`, `running`, `completed`. |
| `action_sequence<MaxActions, Duration, CallbackStorageSize, CallbackStorageAlignment>` | Owns up to `MaxActions` steps. `Duration` is a `castle::chrono::duration` with an integral rep (default `castle::chrono::milliseconds`); its period is the time resolution. |
| `add_call(callable)` | Appends a `void()` call step (`CallFunc`). Returns `status::ok`, `full`, `invalid_config`, or `invalid_callback` (null function pointer or empty `castle::callbacks::function`). |
| `add_delay(duration)` | Appends a delay step (`DelayTime`). Accepts any duration whose unit is a whole multiple of the resolution. Zero is allowed. Returns `invalid_argument` for negative and `out_of_range` for unrepresentable delays. |
| `append(other)` | Copies every step of another sequence with the same `Duration` and storage configuration (composition by flattening). `full` leaves this sequence unchanged. |
| `start()` | Starts or restarts from the first step without executing anything. An empty sequence becomes `completed`. |
| `update()` / `update(elapsed)` | Runs every step that becomes reachable and returns the state. Calls take no time; leftover time carries past an expired delay. Negative `elapsed` counts as zero; values beyond the `Duration` range saturate. |
| `reset()` | Returns to `idle` and rewinds delays; keeps steps. |
| `clear()` | Removes every step and returns to `idle`. |
| `size()`, `empty()`, `state()`, `is_running()`, `is_completed()` | State queries. |
| `next_index()`, `remaining_delay()` | Progress queries; `remaining_delay()` returns `Duration`. |
| `capacity()` | Compile-time `MaxActions`. |

Each `update()` visits every step at most once, so its cost is bounded by `MaxActions`.

## Usage Example
```cpp
#include "castle/patterns/action_sequence.hpp"

using castle::chrono::milliseconds;
using castle::chrono::seconds;

castle::patterns::action_sequence<4U> sequence;   // millisecond resolution
sequence.add_call([]() noexcept { /* open */ });
sequence.add_delay(milliseconds(1500));                  // DelayTime::create(1.5f)
sequence.add_call([]() noexcept { /* close */ });
sequence.start();
sequence.update();                                       // runs "open", begins the delay
sequence.update(seconds(1));                             // 500 ms left
sequence.update(milliseconds(500));                      // delay ends, runs "close", completes
```

Driving it from a clock (choose the clock's resolution so differences convert exactly):
```cpp
using clock = castle::chrono::steady_clock;
castle::patterns::action_sequence<8U, clock::duration> sequence;
auto last = clock::now();
// in the loop:
auto now = clock::now();
sequence.update(now - last);
last = now;
```
See [`samples/patterns/action_sequence.cpp`](../../samples/patterns/action_sequence.cpp) for composition with `append()` and tick carry-over.

## Constraints & Notes
- No heap allocation, exceptions, or RTTI. Capacity, time resolution, and callable storage are compile-time parameters.
- Time units finer than the resolution (for example `microseconds` into a millisecond sequence) do not compile: truncating them would make delays end early or make small `update()` steps vanish. Convert explicitly with `castle::chrono::ceil<duration_type>()` for delays, or pick a finer `Duration`.
- On small 32-bit targets, `castle::chrono::duration<uint32_t, castle::milli>` avoids 64-bit arithmetic.
- Callables must be copy-constructible and fit `CallbackStorageSize`; they are stored in `castle::callbacks::function`.
- Steps can be added only when the sequence is not running. Calling `start()`, `reset()`, `clear()`, `add_*()`, or `append()` from inside a step returns `status::invalid_config`. A step must not destroy the sequence that runs it.
- Editing a completed sequence returns it to `idle`.
- Copying a sequence copies its callables, so each copy runs independently.
- Only one delay is in progress at a time, so the sequence keeps one remaining-time counter; `start()`/`reset()` are O(1).
- Drive time from one context. There is no internal synchronization.
