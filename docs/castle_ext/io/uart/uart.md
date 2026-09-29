# UART

## Overview
`basic_uart` owns a POSIX serial descriptor and applies a raw `termios` configuration for baud rate, data bits, parity, stop bits, flow control and receive timing. It is intended for Linux BSP device nodes such as `/dev/ttyS*`, `/dev/ttyUSB*` and `/dev/ttyAMA*`.

## Header
`#include "castle_ext/io/uart/uart.hpp"`

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
| `data_bits`, `parity`, `stop_bits`, `flow_control` | Serial framing and flow-control options. |
| `uart_config` | Borrowed device path, numeric baud rate, framing, flow control, `VMIN`/`VTIME` and non-blocking setting. |
| `validate_config(config)` | Checks field combinations and baud rates supported by this platform's termios headers. |
| `uart_available` | Compile-time POSIX availability flag. |
| `basic_uart` / `uart` | Move-only owner exposing `open`, `configure`, `close`, `is_open`, `is_configured`, `native_handle`, `configuration`. |
| `read_some`, `write_some`, `write_all` | Byte-stream input/output with explicit transfer counts. |
| `flush_input`, `flush_output`, `flush` | Discards the selected UART queue. |
| `drain` | Waits for queued output to be transmitted. |
| `bytes_available` | Queries pending receive bytes through `FIONREAD` when supported. |

## Usage Example
```cpp
#include "castle_ext/io/uart/uart.hpp"
#include <assert.h>

int main()
{
    castle::io::uart::basic_uart serial;
    const castle::io::uart::uart_config cfg("/dev/ttyS0", 115200U);
    assert(castle::io::uart::validate_config(cfg) == castle::status::ok);

    // On the target BSP, open the configured UART device:
    // assert(serial.open(cfg) == castle::status::ok);
    // const char command[] = {'O', 'K'};
    // castle::size_type sent = 0U;
    // assert(serial.write_all(command, sizeof(command), sent) == castle::status::ok);
    (void)serial;
}
```
See `samples/castle_ext/io/uart/uart.cpp` for a standalone sample.

## Constraints & Notes
- `device_path` is a borrowed null-terminated string; no dynamic string ownership is introduced.
- Baud rates are numeric bits-per-second values mapped to native `B*` termios constants. A rate not exposed by the target headers returns `invalid_config`.
- `read_timeout_deciseconds` maps directly to `VTIME` (tenths of a second); `min_read_bytes` maps to `VMIN`.
- `non_blocking` sets `O_NONBLOCK` at open. Combining non-blocking mode with non-zero `VMIN`/`VTIME` is platform-dependent; choose one receive policy deliberately.
- `flush_*` discards data; `drain()` waits for transmission and can block.
- No mutex is introduced. UART read/write/configure/close calls require external synchronization when shared.
- POSIX termios code is guarded by `CASTLE_USING_POSIX_APIS`.
