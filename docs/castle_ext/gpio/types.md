# GPIO Types

## Overview
Platform-neutral value and configuration types shared by every GPIO backend and facade (`basic_pin`, `basic_port`). Defines the electrical/logical `level`, pin `direction`/`pull`/`drive` settings, a fixed-size `pin_config` aggregate, and small `constexpr` conversion helpers. None of this header depends on any specific platform.

## Header
`#include "castle_ext/gpio/types.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../error/status.md`](../../error/status.md)

## Public API
| API | Description |
|---|---|
| `level` | Electrical/logical pin state: `low` / `high`. Deliberately independent of active-low polarity. |
| `direction` | `input` / `output`. |
| `pull` | Internal pull configuration: `none` / `up` / `down`. |
| `drive` | Output driver mode: `push_pull` / `open_drain` / `open_source`. |
| `pin_config` | Fixed-size pin configuration: `dir`, `pull_mode`, `drive_mode`, `initial`, `initial_valid`. `initial_valid` makes output initialization deterministic without a third sentinel `level`. |
| `pin_config::is_input()`, `is_output()` | Convenience direction predicates. |
| `is_high(level)`, `is_low(level)` | Boolean predicates over a `level`. |
| `to_level(bool)`, `to_bool(level)` | Convert between `bool` and `level`. |
| `invert(level)` | Returns the opposite `level`; shared by every active-low-aware backend so the flip logic is written once. |
| `mask_type` | `uint32_t` bit mask type used by fixed-width GPIO port backends (`basic_port`). |

## Usage Example
See `samples/castle_ext/gpio/pin.cpp` and `samples/castle_ext/gpio/port.cpp`.

```cpp
#include "castle_ext/gpio/types.hpp"

constexpr castle::gpio::pin_config output_cfg(
    castle::gpio::direction::output,
    castle::gpio::level::low,
    castle::gpio::pull::none,
    castle::gpio::drive::push_pull,
    /* has_initial */ true);
```

## Constraints & Notes
- Every type here is a plain aggregate or `enum class`; there is no heap allocation, virtual dispatch, RTTI, or STL dependency.
- `level` intentionally carries no polarity information. Active-low inversion is a backend concern (see [backends](backends.md)), applied uniformly through `invert()`.
- `mask_type` is a fixed `uint32_t`; backends for ports wider than 32 bits are expected to split operations across multiple `basic_port` instances rather than widen this type.

