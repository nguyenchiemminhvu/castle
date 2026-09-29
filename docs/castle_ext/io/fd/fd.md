# File Descriptor Module

## Overview
The FD umbrella header collects common POSIX descriptor controls, move-only descriptor ownership and a fixed-capacity `poll()` event manager. All declarations are exposed in `castle::io::fd`.

## Header
`#include "castle_ext/io/fd/fd.hpp"`

## Dependencies
- [`control.md`](control.md)
- [`descriptor.md`](descriptor.md)
- [`poll.md`](poll.md)

## Public API
This header adds no new runtime state. It re-exports:
- `control.hpp`: `get_status_flags`, `set_status_flags`, `set_nonblocking`, `set_blocking`, descriptor-flag functions, `is_valid_fd`, and `close_fd`.
- `descriptor.hpp`: `open_fd`, `read_fd`, `write_fd`, exact/all transfer functions, `basic_fd` and `descriptor`.
- `poll.hpp`: poll event constants, `basic_fd_event_manager` and `fd_event_manager`.

## Usage Example
```cpp
#include "castle_ext/io/fd/fd.hpp"
#include <assert.h>
#include <fcntl.h>

int main()
{
    castle::io::fd::basic_fd file;
    assert(file.open("/dev/null", O_RDWR) == castle::status::ok);
    assert(file.is_open());
    assert(file.close() == castle::status::ok);
}
```
See `samples/castle_ext/io/fd/fd.cpp` for a standalone sample.

## Constraints & Notes
- The descriptor owner is move-only and closes its descriptor on destruction; the poll manager does not own registered descriptors.
- POSIX declarations are guarded by `CASTLE_USING_POSIX_APIS`; control/descriptor calls return `not_configured` when unavailable, while `poll()` types are omitted.
- No dynamic allocation is used. Poll capacity and callback storage are fixed at compile time.
