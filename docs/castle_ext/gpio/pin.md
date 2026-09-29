# GPIO Pin

## Overview
`basic_pin<Backend>` is a backend-independent, allocation-free wrapper over a single GPIO pin. It forwards every call to a platform adapter (`Backend`) and stores nothing beyond the backend's own native handle, so its size is exactly the size required by that binding. Thread/ISR safety is delegated entirely to the backend: `basic_pin` never introduces a mutex, atomic flag, heap storage, or hidden critical section.

## Header
`#include "castle_ext/gpio/pin.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../error/status.md`](../../error/status.md)
- [`types.md`](types.md)

## Public API
| API | Description |
|---|---|
| `basic_pin<Backend>` | GPIO pin facade templated on a platform adapter. `pin<Backend>` is a convenience alias. |
| `basic_pin()` | Default-constructs an empty native handle. |
| `basic_pin(native_handle_type const&)` | Binds the pin to a platform-native handle. |
| `native_handle()` | Returns the bound native handle (const and mutable overloads). |
| `reset(native_handle_type const&)` | Rebinds the object to another native GPIO handle. |
| `configure(pin_config const&)` | Configures direction/pull/drive/initial state through the backend. |
| `write(level)`, `write(bool)` | Writes a logical GPIO level. |
| `read(level&)`, `read(bool&)` | Reads a logical GPIO level. |
| `write_raw(level)`, `read_raw(level&)` | Writes/reads the physical pin state, ignoring active-low polarity when the platform supports that distinction. |
| `toggle()` | Toggles the logical GPIO output using the backend's native operation. |

## Backend contract
```cpp
struct backend {
    using native_handle_type = ...;

    static castle::status configure(native_handle_type const&, pin_config const&);
    static castle::status write(native_handle_type const&, level);
    static castle::status read(native_handle_type const&, level&);
    static castle::status write_raw(native_handle_type const&, level);
    static castle::status read_raw(native_handle_type const&, level&);
    static castle::status toggle(native_handle_type const&);
};
```
`basic_pin` enforces this contract at compile time: instantiating it with a `Backend` that is missing one of these static functions fails a `static_assert` with a single clear message instead of deep, unrelated template errors inside each method body.

Three backends currently implement this contract — see [backends](backends.md):
- `castle::gpio::mock::backend` — deterministic in-memory backend for host-side samples/tests.
- `castle::gpio::linux_file::backend` — POSIX file-descriptor backend for BSP-exposed `.../value` files.
- `castle::gpio::zephyr::backend` — Zephyr `gpio_dt_spec` backend built on `gpio_pin_*_dt()`.

## Usage Example
See `samples/castle_ext/gpio/pin.cpp`.

```cpp
#include "castle_ext/gpio/gpio.hpp"
#include "castle_ext/gpio/backends/mock.hpp"

using namespace castle::gpio;

mock::pin_state led_state(/* active_low */ false, level::low);
pin<mock::backend> led(&led_state);

led.configure(pin_config(direction::output, level::low, pull::none, drive::push_pull, true));
led.write(true);
led.toggle();
```

## Constraints & Notes
- No heap allocation, virtual dispatch, RTTI, or exceptions anywhere in `basic_pin` or any shipped backend.
- `write`/`read` apply the backend's logical semantics (e.g. device-tree active-low flags on Zephyr); `write_raw`/`read_raw` address the physical pin state when the platform exposes that distinction. Backends that cannot distinguish the two apply the same translation to both (see `linux_file`).
- Every operation returns `castle::status`; check `castle::succeeded(...)` before trusting an output parameter.
- `basic_pin` is copyable/movable like its `native_handle_type` — it owns no resources of its own.

