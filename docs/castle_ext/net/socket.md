# BSD Socket

## Overview
`basic_socket<Protocol>` is the low-level owning BSD socket wrapper for TCP or UDP. It composes CASTLE's move-only `basic_fd` descriptor owner and FD readiness wait helper, while providing typed transport operations and conversion between CASTLE endpoints and native IPv4/IPv6 socket addresses. The BSD calls themselves live in [`detail/native.hpp`](detail/native.md); the class only validates state and arguments and maps results to `castle::status`.

## Header
`#include "castle_ext/net/socket.hpp"`

## Dependencies
- [`castle/core/config.hpp`](../../../include/castle/core/config.hpp)
- [`castle/core/compiler.hpp`](../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../include/castle/error/status.hpp)
- [`castle/utility/move.hpp`](../../../include/castle/utility/move.hpp)
- [`endpoint.hpp`](endpoint.md)
- [`detail/native.hpp`](detail/native.md)
- [`castle_ext/io/fd/descriptor.hpp`](../../../include/castle_ext/io/fd/descriptor.hpp)
- [`castle_ext/io/fd/control.hpp`](../../../include/castle_ext/io/fd/control.hpp)
- [`castle_ext/io/fd/poll.hpp`](../../../include/castle_ext/io/fd/poll.hpp)

## Public API
All names are in `castle::net`.

| API | Description |
|---|---|
| `socket_protocol` | Compile-time transport selector: `tcp` or `udp`. |
| `shutdown_mode` | Selects receive, send, or both directions for `shutdown()`. |
| `sockets_available` | Compile-time boolean reflecting `CASTLE_USING_POSIX_APIS`. |
| `basic_socket<Protocol>` | Move-only RAII socket; copy is deleted. Exposes `protocol` and owns one `castle::io::fd::basic_fd`. |
| `tcp_socket`, `udp_socket` | Aliases for TCP and UDP `basic_socket` specializations. |
| `open(family = ipv4, nonblocking = true, system_error)` | Creates a socket with close-on-exec set atomically where the target supports it. Reopening an open socket returns `invalid_config`. |
| `close(system_error)`, `is_open()`, `native_handle()`, `family()`, `is_nonblocking()` | Lifetime and descriptor-state operations. |
| `set_nonblocking(enabled, system_error)` | Changes `O_NONBLOCK` without replacing the descriptor. |
| `set_reuse_address(enabled, system_error)` | Sets `SO_REUSEADDR`. |
| `set_keep_alive(enabled, system_error)` | Sets `SO_KEEPALIVE`; TCP only. |
| `set_no_delay(enabled, system_error)` | Sets `TCP_NODELAY`; TCP only. |
| `bind(local, system_error)` | Binds to a numeric local endpoint matching the socket family. |
| `connect(remote, system_error)` | Initiates a connection. A pending non-blocking connect (`EINPROGRESS`/`EALREADY`) returns `time_out`; call `finish_connect()`. Any other error is a failure. |
| `finish_connect(timeout_ms = 0, system_error)` | TCP only; waits for writability, then checks `SO_ERROR` and `getpeername()` so a connect that never completed is not reported as success. Zero only checks readiness (`time_out` while pending); negative waits indefinitely. UDP returns `invalid_config`. |
| `listen(backlog = 8, system_error)` | Starts a TCP listener; UDP returns `invalid_config`, and a negative backlog returns `invalid_argument`. |
| `accept(out, remote, system_error)` | Accepts one TCP connection into a closed output socket that inherits the listener's blocking mode. Would-block returns `empty`. |
| `send_some(data, size, bytes_sent, system_error)` | Sends a partial stream write or one UDP datagram on a connected socket. A stream that cannot accept data returns `full`; a short datagram write returns `data_loss`. |
| `send_all(data, size, bytes_sent, system_error)` | TCP only; loops until all bytes are sent or a blocked/error status occurs. It does not wait/retry on `full`. |
| `receive_some(data, capacity, bytes_received, system_error, peer_closed)` | Receives from a connected socket. `peer_closed` (optional `bool*`) distinguishes TCP EOF (`empty`, zero bytes, true) from would-block (`empty`, false). A UDP datagram larger than `capacity` returns `data_loss` with `capacity` bytes; the rest is dropped. |
| `send_to(data, size, remote, bytes_sent, system_error)` | Sends exactly one UDP datagram to an explicit endpoint. |
| `receive_from(data, capacity, bytes_received, remote, system_error)` | Receives one UDP datagram and reports its source. Oversize datagrams return `data_loss`. |
| `wait(events, timeout_ms, revents, system_error)` | Waits through `castle::io::fd::wait_fd()`; timeout is in milliseconds, negative means indefinite. |
| `wait_readable(timeout_ms, system_error)` | Waits until readable without needing the POSIX event constants: `ok` when ready, `time_out` when the wait expired. |
| `local_endpoint(out, system_error)`, `peer_endpoint(out, system_error)` | Queries the local or connected remote endpoint. |
| `shutdown(mode = both, system_error)` | Shuts down one or both socket directions. |

`system_error` is an optional trailing pointer. Native system-call failures return `system_call_error` and place `errno` in this output when supplied. Would-block reads return `empty`; would-block writes return `full`.

## Usage Example
See `samples/castle_ext/net/socket.cpp`.

```cpp
#include "castle_ext/net/socket.hpp"
#include <assert.h>

int main()
{
    using namespace castle::net;
    tcp_socket listener;
    assert(listener.open(address_family::ipv4) == castle::status::ok);
    assert(listener.set_reuse_address() == castle::status::ok);
    assert(listener.bind(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    assert(listener.listen() == castle::status::ok);
    assert(listener.close() == castle::status::ok);
    return 0;
}
```

## Constraints & Notes
- Uses POSIX/BSD socket calls when `CASTLE_USING_POSIX_APIS` is enabled; methods remain declared on unsupported targets and report `not_configured`.
- No internal heap allocation. The descriptor is owned by CASTLE `basic_fd`; destruction closes it.
- Sockets default to non-blocking mode. Set `nonblocking = false` in `open()` for blocking socket calls.
- Sends never raise `SIGPIPE`: `MSG_NOSIGNAL` is used where defined, otherwise `SO_NOSIGPIPE` is set on every new socket. A send to a reset connection returns `system_call_error` (`EPIPE`/`ECONNRESET`).
- Datagram truncation is detected by requesting the real length with `MSG_TRUNC` on `recvfrom()`; a target that defines neither `MSG_TRUNC` nor `ZSOCK_MSG_TRUNC` cannot report it.
- Stream I/O is partial by design. Callers must handle `empty`/`full`, partial byte counts, and readiness according to their event-loop policy.
- Numeric IPv4 and IPv6 endpoints are supported; DNS is intentionally not included.
- On Zephyr, enable BSD sockets (`CONFIG_NET_SOCKETS`), the POSIX-compatible header/function profile used by CASTLE, and the required transport support (`CONFIG_NET_TCP` and/or `CONFIG_NET_UDP`). See the [Zephyr BSD sockets documentation](https://docs.zephyrproject.org/latest/connectivity/networking/api/sockets.html) and [POSIX support overview](https://docs.zephyrproject.org/latest/services/portability/posix/overview/index.html). `CASTLE_USING_POSIX_APIS` detects available POSIX headers; it does not enable Zephyr networking modules by itself. Socket options and less-common BSD behaviors remain target-dependent.
- The wrapper is not internally synchronized. Serialize concurrent operations that mutate or consume the same socket unless the target's socket API contract explicitly allows them.
