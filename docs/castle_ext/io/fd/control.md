# File Descriptor Controls

## Overview
Provides allocation-free wrappers for common POSIX file-descriptor operations: reading and writing status flags, toggling non-blocking mode, toggling close-on-exec, checking whether a descriptor is open, and closing a descriptor. Results use CASTLE's `castle::status` instead of exceptions.

## Header
`#include "castle_ext/io/fd/control.hpp"`

## Dependencies
- [`../../../core/config.md`](../../../core/config.md)
- [`../../../core/compiler.md`](../../../core/compiler.md)
- [`../../../error/status.md`](../../../error/status.md)

## Public API
| API | Description |
|---|---|
| `posix_available` | Compile-time flag indicating whether POSIX APIs are enabled. |
| `get_status_flags(fd, flags, system_error)` | Reads `F_GETFL`; clears `flags` on failure. |
| `set_status_flags(fd, flags, system_error)` | Calls `F_SETFL` with the supplied status flags. |
| `set_nonblocking(fd, enabled, system_error)` | Toggles `O_NONBLOCK`, preserving unrelated status flags. |
| `set_blocking(fd, system_error)` | Convenience wrapper for `set_nonblocking(fd, false, ...)`. |
| `get_descriptor_flags(fd, flags, system_error)` | Reads `F_GETFD`; clears `flags` on failure. |
| `set_descriptor_flags(fd, flags, system_error)` | Calls `F_SETFD` with the supplied descriptor flags. |
| `set_close_on_exec(fd, enabled, system_error)` | Toggles `FD_CLOEXEC`, preserving unrelated descriptor flags. |
| `is_valid_fd(fd)` | Returns whether `F_GETFD` succeeds for the descriptor. |
| `close_fd(fd, system_error)` | Performs exactly one `close()` call. |

All operations return `castle::status`. POSIX system-call failures return `system_call_error`; an optional `system_error` output receives `errno`. An invalid negative descriptor returns `invalid_argument`. When POSIX support is disabled, operations return `not_configured` (after validating negative descriptors).

## Usage Example
```cpp
#include "castle_ext/io/fd/control.hpp"
#include <assert.h>
#include <unistd.h>

int main()
{
    int pipe_fds[2] = {-1, -1};
    assert(::pipe(pipe_fds) == 0);

    assert(castle::io::fd::set_nonblocking(pipe_fds[0]) == castle::status::ok);
    assert(castle::io::fd::set_blocking(pipe_fds[0]) == castle::status::ok);
    assert(castle::io::fd::close_fd(pipe_fds[0]) == castle::status::ok);
    assert(castle::io::fd::close_fd(pipe_fds[1]) == castle::status::ok);
}
```
See `samples/castle_ext/io/fd/control.cpp` for a standalone sample.

## Constraints & Notes
- POSIX declarations are included only under `#if CASTLE_USING_POSIX_APIS`; non-POSIX builds retain status-returning stubs.
- No heap allocation, exceptions, RTTI, virtual functions, or descriptor ownership tracking is introduced.
- `set_nonblocking()` and `set_close_on_exec()` perform read-modify-write sequences; callers must coordinate concurrent flag changes to the same descriptor.
- `close_fd()` deliberately does not retry `EINTR`, because descriptor state after an interrupted close can be platform-dependent and retrying can close a reused descriptor.
- Apart from `close_fd()`, these helpers do not assume ownership or lifetime of the supplied descriptor.
