# castle::filter::debounce_sampling

## Overview

`castle::filter::debounce_sampling` turns a noisy digital signal — a mechanical button, a
GPIO pin, a comparator output, anything that can be read as `bool` on a fixed sampling
interval — into a clean, confirmed, four-stage signal: **cleared → valid → held → repeating**.

It is CASTLE's version of the classic *debounce* building block found in libraries such as
ETL (`etl::debounce`), redesigned around an explicit finite state machine (FSM), compile-time
threshold validation, and optional per-transition callbacks — with no dynamic memory, no
virtual dispatch, and no bitmask tricks.

### Why debouncing is needed

A physical switch or button does not produce a single clean transition from "released" to
"pressed". The metal contacts physically bounce for a few milliseconds, so a naive digital
read during that window looks like:

```text
released ⇒ pressed ⇒ released ⇒ pressed ⇒ pressed ⇒ pressed ⇒ pressed ...
```

If you react to every raw transition, one physical button press can be misread as several.
*Debouncing* is the technique of waiting for the input to be **stable for a minimum number of
consecutive samples** before trusting it. `debounce_sampling` implements exactly that, and
extends it with two more useful, optional stages:

- **valid** — the debounced/confirmed logical state (this is the "is it actually pressed?" answer).
- **held** — the signal has stayed `valid` long enough to be considered a long-press.
- **repeating** — the signal has stayed `held` long enough that it should behave like an
  auto-repeating key (think "held-down arrow key retypes the character every N ticks").

## Header

```cpp
#include "castle/filter/debounce_sampling.hpp"
```

## Namespace

```cpp
namespace castle
{
namespace filter
{
// debounce_state, debounce_sampling
}
}
```

## Design

`debounce_sampling` is modeled as an explicit finite state machine with four named states:

```text
                 ValidCount                 HoldCount                  RepeatCount
              true samples               true samples                true samples
   ┌─────────┐ ───────────────► ┌───────┐ ───────────────► ┌────────┐ ───────────────► ┌────────────┐
   │ cleared │                  │ valid │                  │  held  │                  │ repeating  │
   └─────────┘ ◄─────────────── └───────┘ ◄─────────────── └────────┘ ◄─────────────── └────────────┘
        ▲                    ValidCount false samples (from ANY state)                         │
        └──────────────────────────────────────────────────────────────────────────────────────┘
                                                                              RepeatCount true samples
                                                                     (self-loop: stays `repeating`,
                                                                      re-fires the event periodically)
```

Two design points are easy to miss but important to understand:

1. **A "false" sample is debounced too, using `ValidCount` again.** Releasing a button while it
   is `held` or `repeating` does not instantly drop the state back to `cleared`. It requires
   `ValidCount` consecutive `false` samples first — the exact same confirmation delay used to
   *enter* `valid` in the first place. This means a single opposite-polarity glitch while the
   button is held down will not falsely clear the state.
2. **Only the run-length counter restarts on a raw input change — not the state.** Every time
   the raw sample flips (regardless of what state the FSM is in), the internal counter resets to
   `1` for that new value. The FSM state itself changes only when the counter actually reaches
   one of `ValidCount` / `HoldCount` / `RepeatCount`.
3. **`repeating` is a self-loop.** Once in `repeating`, hitting another `RepeatCount` samples
   does not move to a new state (there is no "state after repeating") — it simply re-fires the
   `on_repeating` event and restarts the counter, which is exactly the auto-repeat behavior you
   want from a held key.

### State reference

| State | Meaning | Reached after |
| --- | --- | --- |
| `debounce_state::cleared` | No confirmed signal. Initial state. | Constructed, or `ValidCount` consecutive `false` samples from any other state. |
| `debounce_state::valid` | The input is confirmed / debounced. | `ValidCount` consecutive identical (`true`) samples from `cleared`. |
| `debounce_state::held` | The confirmed input has been sustained long enough to be a "long press". | An additional `HoldCount` samples since becoming `valid`. Skipped entirely if `HoldCount == 0`. |
| `debounce_state::repeating` | The held input should behave like an auto-repeating key. | An additional `RepeatCount` samples since becoming `held`, then re-fires every further `RepeatCount` samples. Skipped entirely if `RepeatCount == 0`. |

`HoldCount` and `RepeatCount` default to `0`, which disables that stage: with only `ValidCount`
configured, `debounce_sampling` behaves as a plain press/release debouncer. `RepeatCount`
requires `HoldCount > 0` (enforced at compile time — see below) because repeat conceptually
builds on top of hold.

## Template parameters

```cpp
template <
    castle::size_type ValidCount,
    castle::size_type HoldCount = 0U,
    castle::size_type RepeatCount = 0U,
    typename CounterType = uint16_t,
    castle::size_type CallbackStorageSize = castle::inplace_storage_reserved,
    castle::size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class debounce_sampling;
```

### `ValidCount`

Number of consecutive identical samples required to confirm a state change (both entering
`valid` from `cleared`, and falling back to `cleared` from any deeper state). Must be greater
than zero — enforced by `static_assert`.

### `HoldCount`

Additional consecutive samples (counted from the moment the signal became `valid`) required to
enter `held`. `0` (the default) disables the hold stage entirely — the debouncer will only ever
reach `valid`, never `held` or `repeating`.

### `RepeatCount`

Additional consecutive samples (counted from the moment the signal became `held`, and then
again after every re-fire) required to enter, and subsequently keep re-firing, `repeating`. `0`
(the default) disables the repeat stage. Using a non-zero `RepeatCount` while `HoldCount == 0`
is a compile error, because repeat is only meaningful on top of an enabled hold stage.

### `CounterType`

Unsigned integral type used to store the internal run-length counter. Defaults to `uint16_t`
(2 bytes), matching typical embedded debounce counters. A `static_assert` verifies that
`CounterType` can represent the largest configured threshold (`ValidCount`, `HoldCount`, or
`RepeatCount`) without silently wrapping — use a wider type (e.g. `uint32_t`) if any threshold
exceeds what the default can hold.

### `CallbackStorageSize` / `CallbackStorageAlignment`

Inline storage size/alignment for each of the four `castle::callbacks::function<void()>`
callback slots (`on_valid`, `on_held`, `on_repeating`, `on_cleared`). Both default to CASTLE's
standard inline-callback storage (`castle::inplace_storage_reserved` /
`castle::inplace_alignment_default`), which is generous enough for typical capturing lambdas.
Only override these if your callback captures unusually large state.

## Basic usage

```cpp
#include "castle/filter/debounce_sampling.hpp"

// Confirm after 20 consecutive identical samples (e.g. a 20 ms debounce
// window at a 1 ms sampling tick).
castle::filter::debounce_sampling<20U> button;

bool raw_pin_state = read_gpio_pin();
if (button.sample(raw_pin_state))
{
    // A confirmed state transition (press or release) just happened.
    if (button.is_valid())
    {
        // Button is now considered pressed.
    }
    else
    {
        // Button is now considered released.
    }
}
```

`sample()` (or the equivalent `operator()`) is the only method that needs to be called
periodically — typically once per scheduler tick, timer ISR, or polling loop iteration. It
returns `true` exactly on the call where a state transition (or a periodic repeat re-fire)
occurred, and `false` on every other call, so callers that don't use callbacks can simply check
the return value.

## Valid / hold / repeat detection

```cpp
castle::filter::debounce_sampling<5U, 10U, 3U> key;

// Feed samples once per tick, e.g. from a keyboard scan routine.
key.sample(pressed);

if (key.is_valid())     { /* debounced press is confirmed */ }
if (key.is_held())      { /* pressed for >= HoldCount extra ticks: long-press */ }
if (key.is_repeating()) { /* pressed long enough to auto-repeat */ }
```

- `is_valid()` is `true` in `valid`, `held`, *and* `repeating` (it answers "is the confirmed
  signal currently set at all?").
- `is_held()` is `true` in `held` and `repeating` only.
- `is_repeating()` is `true` only in `repeating`.
- `state()` returns the exact `debounce_state` if you need to distinguish all four cases directly.
- `value()` returns the last raw sample observed (useful for diagnostics; it is *not* debounced).

## Callback subscription and removal

Every state transition can optionally invoke a `void()` callback, stored in a
`castle::callbacks::function` — no heap allocation, no virtual dispatch, and a plain
no-op when a callback is left unset:

```cpp
using button_type = castle::filter::debounce_sampling<20U, 1000U, 200U>;
button_type button;

button.on_valid([] { /* pressed (debounced) */ });
button.on_held([] { /* long-press detected */ });
button.on_repeating([] { /* auto-repeat tick while held */ });
button.on_cleared([] { /* released (debounced) */ });

button.sample(read_gpio_pin());
```

A callback is removed (or replaced) simply by assigning a new one — assigning an empty
`callback_type{}` clears it:

```cpp
button.on_held(button_type::callback_type{}); // remove the hold callback
button.clear_callbacks();                     // remove all four at once
```

Registering a new callback for the same event always replaces the previous one; there is no
multi-subscriber fan-out (see "Design rationale: why `function`, not a registry" below).

## Reset

```cpp
void reset(bool initial_sample = false);
```

Restarts the FSM back to `cleared`, clears the run-length counter, and seeds the "last observed
sample" baseline with `initial_sample`. Registered callbacks are preserved — call
`clear_callbacks()` separately if you also want to remove them.

## Integration into a periodic task / scheduler tick

`debounce_sampling` is designed to be driven from exactly one place: a fixed-period sampling
point, such as a timer ISR, an RTOS periodic task, or a super-loop tick.

```cpp
castle::filter::debounce_sampling<20U> button;
bool led_state = false;

// Registered once, e.g. with a 1 ms hardware timer.
void on_scheduler_tick_1ms()
{
    if (button.sample(read_gpio_pin()) && button.is_valid())
    {
        led_state = !led_state;
        write_led_pin(led_state);
    }
}
```

Because `sample()` runs in O(1) time with no branches proportional to elapsed history, it is
safe to call directly from an interrupt context or the tightest loop in a scheduler.

## Embedded notes

- `debounce_sampling` is a value type: no dynamic allocation, no pointers to external objects,
  safe to place in `static`/`.bss`, a stack frame, or another aggregate.
- It is not thread-safe by itself: if `sample()` can be called from more than one execution
  context (e.g. an ISR and a background task), the caller must serialize access.
- Callbacks execute synchronously, inline, on whatever thread/ISR called `sample()` — keep them
  short, exactly like any other ISR-adjacent callback in CASTLE.
- Choose `ValidCount` based on your sampling period and the physical bounce time of your signal
  source (e.g. a 1 ms tick with `ValidCount = 20` yields a 20 ms debounce window).
