# I/O Umbrella

## Overview
The I/O umbrella header collects CASTLE's POSIX descriptor owner and the file, UART, SPI and I2C components. Applications can include a single header for the supported Linux I/O building blocks, or include only the transport they need.

## Header
`#include "castle_ext/io/io.hpp"`

## Dependencies
- [`fd/descriptor.md`](fd/descriptor.md)
- [`fd/control.md`](fd/control.md)
- [`fd/poll.md`](fd/poll.md)
- [`file/linux_file.md`](file/linux_file.md)
- [`uart/uart.md`](uart/uart.md)
- [`spi/spi.md`](spi/spi.md)
- [`i2c/i2c.md`](i2c/i2c.md)

## Public API
This is an umbrella header and adds no runtime types or functions. It exposes:
- `castle::io::fd` — POSIX descriptor control, ownership, byte I/O and `poll()` event dispatch.
- `castle::io::file` — `basic_file` / `linux_file` for files and device nodes.
- `castle::io::uart` — `basic_uart` / `uart` for termios-configured serial ports.
- `castle::io::spi` — `basic_spi` / `spi` for Linux spidev.
- `castle::io::i2c` — `basic_i2c` / `i2c` for Linux i2c-dev.

## Usage Example
```cpp
#include "castle_ext/io/io.hpp"
#include <assert.h>

int main()
{
    castle::io::fd::basic_fd descriptor;
    castle::io::uart::uart_config serial("/dev/ttyS0", 115200U);
    assert(castle::io::uart::validate_config(serial) == castle::status::ok);
    (void)descriptor;
}
```
See `samples/castle_ext/io/io.cpp` for a standalone sample.

## Constraints & Notes
- No component introduces heap allocation, exceptions, RTTI, virtual functions or STL dependencies.
- Paths and buffer pointers are non-owning; callers control their lifetime.
- POSIX functions and headers are guarded by `CASTLE_USING_POSIX_APIS`. SPI and I2C additionally require Linux UAPI headers.
- Device access is synchronous. No hidden threads, callbacks, mutexes, reconnection policy or protocol-specific framing are introduced.
- Individual operations return `castle::status`; inspect transfer counts and optional `errno` outputs as well.
