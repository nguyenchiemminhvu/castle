# File Descriptor Poll Event Manager

## Overview
`basic_fd_event_manager<Capacity>` is a fixed-capacity dispatcher for POSIX file descriptors. It registers an interest mask and callback for each descriptor, waits with `poll()`, and dispatches readiness notifications without heap allocation. Registration state is protected by CASTLE's mutex, while callbacks run outside manager locks. For code that owns a single descriptor and needs no dispatcher, the free function `wait_fd()` waits for one descriptor with a bounded or infinite timeout and no registration state.

## Header
`#include "castle_ext/io/fd/poll.hpp"`

## Dependencies
- [`control.md`](control.md)
- [`../../../core/config.md`](../../../core/config.md)
- [`../../../core/compiler.md`](../../../core/compiler.md)
- [`../../../core/types.md`](../../../core/types.md)
- [`../../../error/status.md`](../../../error/status.md)
- [`../../../callbacks/function.md`](../../../callbacks/function.md)
- [`../../../sync/mutex.md`](../../../sync/mutex.md)
- [`../../../sync/scoped_mutex.md`](../../../sync/scoped_mutex.md)
- [`../../../utility/move.md`](../../../utility/move.md)

## Public API
| API | Description |
|---|---|
| `poll_available` | Compile-time flag indicating whether POSIX `poll()` is available. |
| `poll_read`, `poll_write`, `poll_priority` | Common interest bits mapped to `POLLIN`, `POLLOUT`, and `POLLPRI`. |
| `poll_read_normal`, `poll_read_band`, `poll_write_normal`, `poll_write_band`, `poll_message`, `poll_remove` | Additional platform-defined poll bits; each declaration is available only when its matching system macro exists. |
| `poll_error`, `poll_hangup`, `poll_invalid`, `poll_read_hangup` | Result bits mapped to `POLLERR`, `POLLHUP`, `POLLNVAL`, and (where supported) `POLLRDHUP`. |
| `wait_fd(fd, events, timeout_ms, revents, system_error)` | Free function: waits for one descriptor to report any bit of `events`. Returns `ok` when ready (inspect `revents`, which may carry `poll_error`/`poll_hangup`), `time_out` when `timeout_ms` expired, `invalid_argument` for a negative descriptor and `system_call_error` when `poll()` fails. `timeout_ms == 0` checks without blocking, a negative value waits indefinitely. |
| `basic_fd_event_manager<Capacity, CallbackStorageSize>` | Manager with a compile-time registration limit and callback inline-storage size. |
| `fd_event_manager` | Alias for `basic_fd_event_manager<16U>`. |
| `add_fd(fd, events, callback, user_data, name)` | Registers a unique descriptor; name and user data are non-owning. |
| `remove_fd(fd)` | Removes a registration but does not close the descriptor. |
| `enable_fd(fd)`, `disable_fd(fd)` | Enables/disables monitoring without removing the registration. |
| `modify_events(fd, events)` | Changes the interest mask for a registered descriptor. |
| `has_fd(fd)` | Checks whether a descriptor is registered. |
| `get_fd_count()`, `get_enabled_fd_count()` | Returns registration counts. |
| `get_fd_info(fd, info)` | Retrieves the descriptor's mask, name, context, and enabled state. |
| `wait(timeout_ms, ready_count, system_error)` | Calls `poll()` and queues ready callbacks without invoking them. Negative timeout means infinite wait. |
| `process_events(processed_count)`, `process_events()` | Consumes the pending batch and invokes its callbacks, optionally returning a count. |
| `wait_and_process(timeout_ms, processed_count, system_error)`, `wait_and_process()` | Convenience method combining `wait()` and `process_events()`; the no-argument overload waits indefinitely and discards the count. |
| `pending_event_count()` | Returns the number of queued callbacks. |
| `clear()` | Removes all registrations and discards any queued callbacks. |

Every operation that can fail returns `castle::status`. A manager wait timeout is a successful operation with zero ready callbacks. `system_error`, when supplied, receives `errno` only when `poll()` fails. `wait_fd()` is the exception to the timeout rule: it has no callback batch to report an empty result through, so a timeout is the distinct status `time_out`, with `revents` set to zero.

## Usage Example
```cpp
#include "castle_ext/io/fd/poll.hpp"
#include <assert.h>
#include <unistd.h>

int main()
{
    int pipe_fds[2] = {-1, -1};
    assert(::pipe(pipe_fds) == 0);

    int callback_count = 0;
    castle::io::fd::fd_event_manager manager;
    assert(manager.add_fd(pipe_fds[0], castle::io::fd::poll_read,
        [](int fd, short revents, void* context)
        {
            if ((revents & castle::io::fd::poll_read) != 0)
            {
                char byte = 0;
                (void)::read(fd, &byte, 1U);
                ++*static_cast<int*>(context);
            }
        }, &callback_count) == castle::status::ok);

    const char byte = 'x';
    assert(::write(pipe_fds[1], &byte, 1U) == 1);
    castle::size_type processed = 0U;
    assert(manager.wait_and_process(1000, processed) == castle::status::ok);
    assert(processed == 1U && callback_count == 1);

    manager.clear();
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
}
```

`wait_fd()` waits for a single descriptor without constructing a manager:
```cpp
#include "castle_ext/io/fd/poll.hpp"
#include <unistd.h>

bool is_readable_within(int descriptor, int timeout_ms)
{
    short revents = 0;
    int system_error = 0;
    const castle::status result =
        castle::io::fd::wait_fd(descriptor, castle::io::fd::poll_read, timeout_ms, revents, &system_error);
    if (result == castle::status::time_out)
    {
        return false; // nothing arrived in time; revents == 0
    }
    // ok: ready. A hang-up or error is reported through revents, not through the status.
    return result == castle::status::ok && (revents & castle::io::fd::poll_read) != 0;
}
```
See `samples/castle_ext/io/fd/poll.cpp` for a standalone manager sample. `wait_fd()` is also what the [CAN SocketCAN backend](../can/backends/socket_can.md) uses to implement its `send()` and `receive()` timeouts.

## Constraints & Notes
- `Capacity` is fixed at compile time and must be greater than zero. Registration and lookup are linear in `Capacity`; `poll()` receives at most `Capacity` descriptors.
- Callback objects use `castle::callbacks::function`; each must be copy-constructible and fit in `CallbackStorageSize` bytes and its alignment. The default storage size is CASTLE's in-place function storage.
- A callback is copied into the pending batch. For persistent mutable state, use `user_data` or capture a stable pointer/reference rather than mutating a by-value mutable lambda capture.
- Names and `user_data` are non-owning. The caller must keep the pointed-to objects alive while registrations or callbacks can use them.
- `wait_fd()` is stateless, takes no lock and may be called concurrently from any thread. It performs exactly one `poll()` call: an interrupted wait (`EINTR`) is reported as `system_call_error` with `errno` in `system_error` and is not retried, so the caller keeps control of the remaining timeout. `revents` and `system_error` are cleared on entry.
- `ok` from `wait_fd()` means `poll()` reported at least one event; it does not mean the requested one. A closed peer or failed socket reports `ok` with `poll_hangup`, `poll_error` or `poll_invalid` in `revents`, so check the bits before reading or writing.
- The manager is thread-safe for its own registration and queue state. Only one blocking `wait()` is allowed at a time; overlapping waits or calling `wait()` before dispatching a pending batch returns `invalid_config`.
- Registrations changed while `poll()` is blocked are generation-checked and their stale readiness is discarded. Once a callback has been queued by a successful `wait()`, it is a committed snapshot: later `remove_fd()`, `disable_fd()`, or `modify_events()` calls do not cancel it. Call `clear()` to discard the queued batch before closing a descriptor that may be referenced by it.
- Callbacks are invoked with no manager mutex held and may re-enter manager methods.
- The manager does not own, close, duplicate, or validate the lifetime of registered descriptors. Closing/reusing a descriptor remains the caller's responsibility.
- POSIX `poll()` support is gated by `CASTLE_USING_POSIX_APIS`. When unavailable, `wait_fd()`, the manager type and poll constants are not declared by this header; `poll_available` is false.
