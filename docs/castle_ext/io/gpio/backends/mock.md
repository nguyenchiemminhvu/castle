# GPIO Mock Backend

## Overview
Provides deterministic, allocation-free in-memory pin and port backends for host-side samples and unit tests. No operating-system or device-tree support is required.

## Header
`#include "castle_ext/io/gpio/backends/mock.hpp"`

## Dependencies
- `castle/error/status.hpp`
- [`../types.md`](../types.md)

## Public API
| API | Description |
|---|---|
| `pin_state` | Stores physical level, the last `pin_config`, configured state and active-low polarity. |
| `backend::configure(handle, config)` | Records pin configuration. |
| `backend::write/read` | Logical pin writes/reads that apply active-low translation. |
| `backend::write_raw/read_raw` | Physical pin access without polarity translation. |
| `backend::toggle(handle)` | Toggles the stored physical state. |
| `port_state` | Stores physical port bits and an active-low mask. |
| `port_backend::read/read_raw/write` | Logical, physical and full-mask port access. |
| `port_backend::write_masked/set/clear/toggle` | Masked bit operations on the in-memory port. |

## Usage Example
```cpp
#include "castle_ext/io/gpio/backends/mock.hpp"
#include <assert.h>

int main()
{
    castle::io::gpio::mock::pin_state state(true);
    const castle::io::gpio::level output = castle::io::gpio::level::high;
    assert(castle::io::gpio::mock::backend::write(&state, output) == castle::status::ok);
    castle::io::gpio::level input = castle::io::gpio::level::low;
    assert(castle::io::gpio::mock::backend::read(&state, input) == castle::status::ok);
    assert(input == output);
}
```
See `samples/castle_ext/io/gpio/backends/mock.cpp` for a standalone sample.

## Constraints & Notes
- State objects are owned by the caller; backend handles are non-owning pointers.
- The backend is deterministic and has no hidden allocation, locking or hardware side effects.
- It is intended for simulation and tests, not for production GPIO access.
