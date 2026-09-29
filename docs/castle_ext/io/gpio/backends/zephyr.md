# GPIO Zephyr Backend

## Overview
Adapts Zephyr's `gpio_dt_spec` device-tree GPIO handle to CASTLE's static pin backend contract. Logical operations honor active-low device-tree flags, while raw operations access the physical level directly.

## Header
`#include "castle_ext/io/gpio/backends/zephyr.hpp"`

## Dependencies
- `castle/error/status.hpp`
- [`../types.md`](../types.md)
- Zephyr `<zephyr/drivers/gpio.h>` when `CASTLE_EXT_ENABLE_GPIO_ZEPHYR` is defined.

## Public API
| API | Description |
|---|---|
| `backend::native_handle_type` | `gpio_dt_spec`, the Zephyr device-tree GPIO binding. |
| `backend::configure(handle, config)` | Translates CASTLE direction, initial value, pull and drive options to Zephyr flags. |
| `backend::write/read` | Logical GPIO operations using `gpio_pin_set_dt` and `gpio_pin_get_dt`. |
| `backend::write_raw/read_raw` | Physical operations using Zephyr raw GPIO calls. |
| `backend::toggle(handle)` | Logical toggle using `gpio_pin_toggle_dt`. |

The backend is defined only when `CASTLE_EXT_ENABLE_GPIO_ZEPHYR` is set before inclusion and the Zephyr SDK headers are on the include path. Otherwise only a forward declaration is provided.

## Usage Example
```cpp
#define CASTLE_EXT_ENABLE_GPIO_ZEPHYR
#include "castle_ext/io/gpio/backends/zephyr.hpp"
#include "castle_ext/io/gpio/pin.hpp"

// In a Zephyr application, bind this facade to a board-specific gpio_dt_spec:
// static const gpio_dt_spec led_spec = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// castle::io::gpio::pin<castle::io::gpio::zephyr::backend> led(led_spec);
```
See `samples/castle_ext/io/gpio/backends/zephyr.cpp` for the host-side feature-guard sample.

## Constraints & Notes
- Requires a Zephyr build, the GPIO driver API and a valid device-tree `gpio_dt_spec`.
- Device readiness is checked before backend operations; not-ready devices return `not_configured`.
- Zephyr return codes are mapped to `castle::status`; the native Zephyr error code is not separately exposed.
- This backend has no additional storage, dynamic allocation or synchronization beyond Zephyr's GPIO driver behavior.
