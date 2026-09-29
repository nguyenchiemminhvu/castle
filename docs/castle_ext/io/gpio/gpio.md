# GPIO Umbrella

## Overview
Collects the platform-neutral GPIO pin and fixed-width port facades together with their shared electrical-level and configuration types. Platform backends are intentionally included separately so applications only pull in a backend when they need it.

## Header
`#include "castle_ext/io/gpio/gpio.hpp"`

## Dependencies
- `castle/core/compiler.hpp`
- `castle/core/traits.hpp`
- `castle/error/status.hpp`
- [`types.md`](types.md)
- [`pin.md`](pin.md)
- [`port.md`](port.md)

## Public API
| API | Description |
|---|---|
| `level`, `direction`, `pull`, `drive` | Platform-neutral GPIO state and electrical configuration enums. |
| `pin_config` | Fixed-size pin direction, initial output level, pull and drive configuration. |
| `basic_pin<Backend>` / `pin<Backend>` | Zero-overhead facade over a static pin backend contract. |
| `basic_port<Backend>` / `port<Backend>` | Fixed-width mask facade over a static port backend contract. |
| `is_high`, `is_low`, `to_level`, `to_bool`, `invert` | Compile-time-friendly level conversion helpers. |
| `mask_type` | `uint32_t` port mask type. |

## Usage Example
```cpp
#include "castle_ext/io/gpio/gpio.hpp"
#include "castle_ext/io/gpio/backends/mock.hpp"
#include <assert.h>

int main()
{
    castle::io::gpio::mock::pin_state state;
    castle::io::gpio::pin<castle::io::gpio::mock::backend> led(&state);
    assert(led.configure(castle::io::gpio::pin_config(
        castle::io::gpio::direction::output)) == castle::status::ok);
    assert(led.write(castle::io::gpio::level::high) == castle::status::ok);
}
```
See `samples/castle_ext/io/gpio/gpio.cpp` for a standalone sample.

## Constraints & Notes
- The facades store only their backend-native handle and introduce no heap allocation, virtual dispatch, exceptions, RTTI or STL dependency.
- Backends are compile-time template parameters and must implement the documented static contract.
- The facade adds no synchronization; thread/ISR safety belongs to the selected backend and application.
- Include a backend header explicitly, for example `backends/mock.hpp`, `backends/linux_file.hpp` or `backends/zephyr.hpp`.
