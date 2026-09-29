# GPIO Backends

## Overview
A backend is a small struct of `static` functions implementing the `basic_pin`/`basic_port` contract (see [pin](pin.md) and [port](port.md)) for one platform. Castle ships three pin backends and one port backend; all are header-only and compiled out entirely unless their feature macro is defined, so including `castle_ext/gpio/backends/*.hpp` is always safe even on a host build or unit test binary that enables none of them.

## Headers
- `#include "castle_ext/gpio/backends/mock.hpp"` — always available, no feature macro required.
- `#include "castle_ext/gpio/backends/linux_file.hpp"` — requires `CASTLE_EXT_ENABLE_GPIO_LINUX_FILE`.
- `#include "castle_ext/gpio/backends/zephyr.hpp"` — requires `CASTLE_EXT_ENABLE_GPIO_ZEPHYR`.

## `castle::gpio::mock`
Deterministic, allocation-free, in-memory backend intended for host-side samples and unit tests. Always compiled in (no feature macro).

| API | Description |
|---|---|
| `pin_state` | In-memory state backing `backend`: physical `level`, last applied `pin_config`, a `configured` flag, and an `active_low` flag. |
| `backend` | Implements the `basic_pin` contract over `pin_state*`, applying `active_low` the same way a real backend would. |
| `port_state` | In-memory state backing `port_backend`: physical `mask_type` and an `active_low_mask`. |
| `port_backend` | Implements the `basic_port` contract over `port_state*`. `set`/`clear` are expressed in terms of `write_masked`. |

## `castle::gpio::linux_file`
POSIX file-descriptor backend for BSPs that expose a single GPIO as a user-space value file (the common sysfs-style `.../gpioNN/value` convention). Uses `open`/`read`/`write`/`close` directly — no `<string>`, streams, or filesystem classes. Each `read`/`write` call opens and closes the file; there is no persistent file descriptor, so this backend does not assume the BSP keeps one open.

`configure()` always returns `castle::status::not_configured`: direction/pull/drive configuration is BSP-owned (set up once, outside this backend, typically by the kernel driver or a device tree overlay), and a generic value file has no portable way to change that electrical configuration.

`native_handle_type` is `pin_handle { const char* path; bool active_low; }`.

Requires `#define CASTLE_EXT_ENABLE_GPIO_LINUX_FILE` before including the header; without it, `linux_file::backend` is only forward-declared so the header stays parseable (and the umbrella `gpio.hpp` stays includable) on toolchains that never enable it.

## `castle::gpio::zephyr`
Backend for Zephyr's `gpio_dt_spec` device-tree bindings. `write`/`read` use Zephyr's logical operations (`gpio_pin_set_dt` / `gpio_pin_get_dt`), so active-low device tree flags are honored automatically. `write_raw`/`read_raw` use `gpio_pin_set_raw` / `gpio_pin_get_raw` so the active-low flag is not applied twice. `toggle()` uses `gpio_pin_toggle_dt`, Zephyr's own atomic logical toggle.

`native_handle_type` is `gpio_dt_spec`. `configure()` translates `pin_config` into Zephyr's `GPIO_INPUT`/`GPIO_OUTPUT`/`GPIO_OUTPUT_ACTIVE`/`GPIO_OUTPUT_INACTIVE`/`GPIO_PULL_UP`/`GPIO_PULL_DOWN`/`GPIO_OPEN_DRAIN`/`GPIO_OPEN_SOURCE` flags and calls `gpio_pin_configure_dt`.

Requires `#define CASTLE_EXT_ENABLE_GPIO_ZEPHYR` before including the header (and `<zephyr/drivers/gpio.h>` on the include path); without it, `zephyr::backend` is only forward-declared.

## Writing a new backend
1. Define `native_handle_type` (a value small enough to copy freely — it becomes the entire size of `basic_pin`/`basic_port`).
2. Implement the required static functions from the [pin](pin.md) or [port](port.md) contract, returning `castle::status` for every path.
3. If the platform distinguishes logical vs. physical level (active-low polarity), apply the translation in `write`/`read` only, and leave `write_raw`/`read_raw` untranslated — reuse `castle::gpio::invert(level)` from `types.hpp` rather than re-deriving the flip.
4. Gate platform-specific includes behind your own feature macro (mirroring `CASTLE_EXT_ENABLE_GPIO_ZEPHYR`/`CASTLE_EXT_ENABLE_GPIO_LINUX_FILE`) so the header stays parseable when the platform SDK is not on the include path.
5. A missing or mis-signatured static function is caught immediately: instantiating `basic_pin<YourBackend>` or `basic_port<YourBackend>` fails a single `static_assert` naming the full contract, instead of producing errors deep inside unrelated method bodies.

## Constraints & Notes
- No backend allocates, throws, uses RTTI, or depends on the STL.
- Thread/ISR safety is the backend's responsibility; `basic_pin`/`basic_port` add no synchronization of their own (see [pin](pin.md)).

