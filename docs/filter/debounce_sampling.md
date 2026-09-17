# DebounceSampling

## Overview
A fixed-threshold debounce, hold, and auto-repeat finite-state machine for sampled boolean signals. Use it for buttons, switches, GPIO inputs, or other noisy digital sources that are sampled periodically.

## Header
`#include "castle/filter/debounce_sampling.hpp"`

## Dependencies
- [`../algorithm/algorithm.md`](../algorithm/algorithm.md)
- `castle/callbacks/function.hpp`
- `castle/core/type_ranges.hpp`

## Public API
| API | Description |
| --- | --- |
| `enum class debounce_state` | States: `cleared`, `valid`, `held`, `repeating`. |
| `debounce_sampling<...>` | Fixed-capacity debouncer with compile-time thresholds and inline callback storage. |
| `sample` / `operator()` | Feed one raw sample; returns `true` when a state transition or repeat event occurs. |
| `reset` | Clears the state machine back to `cleared` and resets the sample baseline. |
| `state`, `value` | Inspect the current FSM state and last raw sample value. |
| `is_valid`, `is_held`, `is_repeating` | Convenience state predicates. |
| `on_valid`, `on_held`, `on_repeating`, `on_cleared` | Register per-transition callbacks stored in `castle::callbacks::function`. |
| `clear_callbacks` | Removes all registered callbacks. |

## Usage Example
```cpp
// See: samples/sample_debounce_sampling.cpp
#include "castle/filter/debounce_sampling.hpp"

castle::filter::debounce_sampling<5U, 10U, 3U> key;
key.sample(true);
```

## Constraints & Notes
- No heap allocation; callbacks use inline `castle::callbacks::function` storage.
- `ValidCount` must be greater than zero.
- `RepeatCount` requires `HoldCount > 0`.
- `CounterType` must be an unsigned integral type large enough for the configured thresholds.
- Release transitions are debounced with the same `ValidCount` threshold used for press transitions.
