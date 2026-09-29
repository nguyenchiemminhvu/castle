# GPIO Port

## Overview
`basic_port<Backend>` is an allocation-free facade for backends that expose a whole fixed-width GPIO bank/port at once (e.g. a 16- or 32-pin register), rather than one pin at a time. It mirrors `basic_pin`'s design: a thin forwarding wrapper that stores only the backend's native handle.

## Header
`#include "castle_ext/gpio/port.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../error/status.md`](../../error/status.md)
- [`types.md`](types.md)

## Public API
| API | Description |
|---|---|
| `basic_port<Backend>` | GPIO port facade templated on a platform adapter. `port<Backend>` is a convenience alias. |
| `basic_port()` | Default-constructs an empty native handle. |
| `basic_port(native_handle_type const&)` | Binds the port to a platform-native handle. |
| `native_handle()` | Returns the bound native handle. |
| `reset(native_handle_type const&)` | Rebinds the object to another native port handle. |
| `read(mask_type&)` | Reads the port's logical bit mask. |
| `read_raw(mask_type&)` | Reads the port's physical bit mask. |
| `write(mask_type)` | Writes the full port value. |
| `write_masked(mask_type mask, mask_type value)` | Writes only the bits selected by `mask`. |
| `set(mask_type)`, `clear(mask_type)`, `toggle(mask_type)` | Set, clear, or toggle the bits selected by `mask`. |

## Backend contract
```cpp
struct backend {
    using native_handle_type = ...;
    static castle::status read(native_handle_type const&, mask_type&);
    static castle::status read_raw(native_handle_type const&, mask_type&);
    static castle::status write(native_handle_type const&, mask_type);
    static castle::status write_masked(native_handle_type const&, mask_type, mask_type);
    static castle::status set(native_handle_type const&, mask_type);
    static castle::status clear(native_handle_type const&, mask_type);
    static castle::status toggle(native_handle_type const&, mask_type);
};
```
`basic_port` enforces this contract at compile time with the same `static_assert`-based diagnostic as `basic_pin`.

`castle::gpio::mock::port_backend` (see [backends](backends.md)) is the reference implementation and the one used by the unit tests and samples. It is a good starting point for a real port backend (e.g. a Zephyr `gpio_port_*` or a Linux `gpiochip` bank adapter).

## Usage Example
See `samples/castle_ext/gpio/port.cpp`.

```cpp
#include "castle_ext/gpio/gpio.hpp"
#include "castle_ext/gpio/backends/mock.hpp"

using namespace castle::gpio;

mock::port_state bank_state;
port<mock::port_backend> bank(&bank_state);

bank.write(0x00FFU);
bank.set(0x0100U);
bank.clear(0x0001U);
```

## Constraints & Notes
- No heap allocation, virtual dispatch, RTTI, or exceptions.
- `mask_type` is `uint32_t`; a backend for a port wider than 32 bits should split the bank across multiple `basic_port<Backend>` instances.
- `set`/`clear` are expressible purely in terms of `write_masked` (`set(mask) == write_masked(mask, mask)`, `clear(mask) == write_masked(mask, 0)`); a backend without a dedicated atomic set/clear register may implement them that way, as `castle::gpio::mock::port_backend` does.

