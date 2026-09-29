# I2C

## Overview
`basic_i2c` owns a Linux i2c-dev adapter descriptor and binds it to one 7-bit or 10-bit target address. It supports direct byte-stream operations and combined write-then-read transactions with a repeated START via `I2C_RDWR`, suitable for register-addressed devices without baking any device-specific register map into CASTLE.

## Header
`#include "castle_ext/io/i2c/i2c.hpp"`

## Dependencies
- `castle/core/config.hpp`
- `castle/core/compiler.hpp`
- `castle/core/types.hpp`
- `castle/error/status.hpp`
- `castle/utility/move.hpp`
- [`../fd/descriptor.md`](../fd/descriptor.md)

## Public API
| API | Description |
|---|---|
| `i2c_config` | Borrowed adapter path, target address and 7/10-bit address selection. |
| `validate_config(config)` | Checks path and target-address range. |
| `i2c_available` | Compile-time Linux i2c-dev availability flag. |
| `basic_i2c` / `i2c` | Move-only owner exposing `open`, `configure`, `close`, `is_open`, `is_configured`, `native_handle`, `configuration`. |
| `read_some`, `write_some` | Performs direct byte-stream reads/writes against the selected target. |
| `write_read(tx, tx_size, rx, rx_size, system_error)` | Performs one combined two-message `I2C_RDWR` transaction. |
| `get_functionality(flags, system_error)` | Reads adapter capability flags via `I2C_FUNCS`. |

## Usage Example
```cpp
#include "castle_ext/io/i2c/i2c.hpp"
#include <assert.h>

int main()
{
    castle::io::i2c::basic_i2c bus;
    const castle::io::i2c::i2c_config cfg("/dev/i2c-1", 0x42U);
    assert(castle::io::i2c::validate_config(cfg) == castle::status::ok);

    // On a BSP with the target attached to this adapter:
    // assert(bus.open(cfg) == castle::status::ok);
    // const unsigned char reg = 0xFD;
    // unsigned char value[2] = {};
    // assert(bus.write_read(&reg, 1U, value, sizeof(value)) == castle::status::ok);
    (void)bus;
}
```
See `samples/castle_ext/io/i2c/i2c.cpp` for a standalone sample.

## Constraints & Notes
- Requires Linux i2c-dev UAPI headers and an enabled `/dev/i2c-*` adapter; unsupported builds return `not_configured`.
- 7-bit addresses are limited to `0x00..0x7F`; 10-bit addresses to `0x000..0x3FF`. Reserved-address policy is left to the application and adapter.
- `write_read` requires non-null buffers and non-zero phase lengths; each phase is limited to 65535 bytes by `struct i2c_msg`.
- `I2C_RDWR` requires adapter support for raw I2C transactions. A driver may reject it even when ordinary SMBus operations are available.
- The class is synchronous and not internally synchronized. Use external locking when sharing an adapter/device instance.
