# SPI

## Overview
`basic_spi` owns a Linux spidev descriptor and configures SPI mode, word width, clock speed and supported controller flags. Its `transfer()` method issues one synchronous full-duplex `SPI_IOC_MESSAGE(1)` transaction without allocating a transfer queue.

## Header
`#include "castle_ext/io/spi/spi.hpp"`

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
| `mode` | SPI mode 0, 1, 2 or 3. |
| `spi_config` | Borrowed device path, mode, bits per word, maximum clock and controller flags. |
| `transfer_config` | Optional per-transfer delay, word width, speed and chip-select change. Zero width/speed inherit the device configuration. |
| `validate_config(config)` | Checks path, mode, word width and speed. |
| `spi_available` | Compile-time Linux spidev availability flag. |
| `basic_spi` / `spi` | Move-only owner exposing `open`, `configure`, `close`, `is_open`, `is_configured`, `native_handle`, `configuration`. |
| `transfer(tx, rx, size, options, system_error)` | Executes one full-duplex transfer; either buffer may be null for one-way operation. |
| `read_some`, `write_some` | Raw single-direction POSIX operations (not a substitute for full-duplex `transfer`). |

## Usage Example
```cpp
#include "castle_ext/io/spi/spi.hpp"
#include <assert.h>

int main()
{
    castle::io::spi::basic_spi bus;
    const castle::io::spi::spi_config cfg("/dev/spidev0.0",
        castle::io::spi::mode::mode0, 8U, 1000000U);
    assert(castle::io::spi::validate_config(cfg) == castle::status::ok);

    // On a BSP exposing this spidev node:
    // assert(bus.open(cfg) == castle::status::ok);
    // const unsigned char tx[] = {0x9FU};
    // unsigned char rx[sizeof(tx)] = {};
    // assert(bus.transfer(tx, rx, sizeof(tx)) == castle::status::ok);
    (void)bus;
}
```
See `samples/castle_ext/io/spi/spi.cpp` for a standalone sample.

## Constraints & Notes
- Requires Linux spidev UAPI headers and a kernel driver exposing `/dev/spidev*`; otherwise `spi_available` is false and hardware methods return `not_configured`.
- `transfer()` is synchronous and uses one stack-allocated `spi_ioc_transfer`; length is bounded by the Linux ioctl result and transfer structure.
- The driver/controller may reject unsupported mode flags or word widths; such failures return `system_call_error` and expose `errno` when requested.
- `read_some` and `write_some` produce separate single-direction SPI operations. Use `transfer()` for clocked full-duplex exchanges and chip-select control.
- No heap allocation or internal locking; serialize access externally when a bus is shared.
