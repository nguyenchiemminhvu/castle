# File Descriptor Owner and I/O

## Overview
Provides the common allocation-free POSIX descriptor layer used by the file, UART, SPI and I2C components. The free functions report byte counts for individual or complete transfers; `basic_fd` provides move-only RAII ownership without reference counting or hidden synchronization.

## Header
`#include "castle_ext/io/fd/descriptor.hpp"`

## Dependencies
- `castle/core/config.hpp`
- `castle/core/compiler.hpp`
- `castle/core/types.hpp`
- `castle/error/status.hpp`
- [`control.md`](control.md)

## Public API
| API | Description |
|---|---|
| `descriptor_available` | Compile-time indication that POSIX descriptor operations are available. |
| `open_fd(path, flags, out_fd, permissions, system_error)` | Opens a path and returns the descriptor through `out_fd`. |
| `read_fd(fd, buffer, capacity, bytes_read, system_error)` | Performs one read; returns `empty` on EOF or non-blocking no-data. |
| `write_fd(fd, buffer, size, bytes_written, system_error)` | Performs one write; returns `full` on non-blocking back-pressure. |
| `read_exact_fd(...)` | Reads until the requested size is reached or a non-success result occurs. |
| `write_all_fd(...)` | Writes until all bytes are accepted or a non-success result occurs. |
| `basic_fd` / `descriptor` | Move-only owner with `open`, `close`, `release`, `reset`, read/write, and flag-control methods. |

Every byte-transfer function writes the actual transfer count to its output parameter. An optional `system_error` receives `errno` for system-call failures. The `*_all` functions do not busy-wait and preserve partial counts.

## Usage Example
```cpp
#include "castle_ext/io/fd/descriptor.hpp"
#include <assert.h>
#include <fcntl.h>

int main()
{
    castle::io::fd::basic_fd device;
    assert(device.open("/dev/null", O_RDWR) == castle::status::ok);
    const char tx[] = {'C', 'A'};
    castle::size_type sent = 0U;
    assert(device.write_all(tx, sizeof(tx), sent) == castle::status::ok);
    assert(sent == sizeof(tx));
    assert(device.close() == castle::status::ok);
}
```
See `samples/castle_ext/io/fd/descriptor.cpp` for a standalone sample.

## Constraints & Notes
- No heap allocation, exceptions, RTTI, virtual functions, STL dependency or internal locking.
- The descriptor constructor adopts ownership; do not pass a descriptor still owned elsewhere.
- The owner is move-only. `release()` transfers ownership without closing.
- A single read/write may be partial. Exact/all helpers stop on the first reported error and never spin on `EAGAIN`.
- `close()` clears ownership before calling `close(2)` and does not retry an interrupted close.
- POSIX calls and headers are guarded by `CASTLE_USING_POSIX_APIS`; non-POSIX operations return `not_configured`.
