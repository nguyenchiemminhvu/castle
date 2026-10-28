# Network Client

## Overview
`basic_client<Protocol>` is a small TCP/UDP client facade around `basic_socket`. It provides direct caller-buffer I/O as well as an optional inline receive callback. `process_once()` waits for readability, receives into a compile-time-sized buffer, and invokes the callback without allocating memory.

## Header
`#include "castle_ext/net/client.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../include/castle/core/compiler.hpp)
- [`castle/core/config.hpp`](../../../include/castle/core/config.hpp)
- [`castle/core/types.hpp`](../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../include/castle/error/status.hpp)
- [`castle/callbacks/function.hpp`](../../../include/castle/callbacks/function.hpp)
- [`castle/utility/forward.hpp`](../../../include/castle/utility/forward.hpp)
- [`castle/utility/move.hpp`](../../../include/castle/utility/move.hpp)
- [`socket.hpp`](socket.md)
- [`castle/callbacks/callback_dispatcher.hpp`](../../../callbacks/callback_dispatcher.md)

## Public API
All names are in `castle::net`.

| API | Description |
|---|---|
| `basic_client<Protocol, ReceiveBufferSize = 256, CallbackStorageSize = inplace_storage_reserved>` | Move-only client with a fixed receive buffer and an inline callback. Both capacities must be greater than zero. |
| `tcp_client<ReceiveBufferSize, CallbackStorageSize>` / `udp_client<...>` | TCP and UDP aliases. |
| `open(family = ipv4, nonblocking = true, system_error)` | Opens the underlying socket. |
| `bind(local, system_error)` | Opens with the endpoint family when necessary, then binds. |
| `connect(remote, timeout_ms = 0, nonblocking = true, system_error)` | Connects to a numeric peer. A pending non-blocking connect is completed within `timeout_ms` (zero does not wait, negative waits forever). Returns `time_out` while still pending and leaves the socket open for `socket().finish_connect()`. Any other failure closes a socket this call opened, so a retry may use any address family. |
| `close(system_error)`, `is_open()`, `native_handle()`, `socket()` | Lifetime, descriptor, and underlying socket access. |
| `set_nonblocking(enabled, system_error)` | Changes socket blocking mode. |
| `send_some(...)`, `send_all(...)`, `send_to(...)` | Connected send, TCP send-all, and explicit UDP datagram send. |
| `receive_some(..., system_error, peer_closed)` / `receive_from(...)` | Receive into caller-owned storage; optional `peer_closed` distinguishes TCP EOF from would-block. UDP datagrams can report their source endpoint. |
| `set_receive_callback(callable)` | Installs a callable with signature `void(endpoint const&, uint8_t const*, size_type)`. Returns `invalid_callback` for a null function pointer; a callable too large for `CallbackStorageSize` fails to compile (`static_assert`). |
| `clear_receive_callback()` | Removes the receive callback. |
| `process_once(timeout_ms = 0, system_error)` | Processes at most one readable event. Zero polls; negative timeout waits indefinitely. |

`process_once()` returns `ok` after a chunk or datagram was received, `time_out` when nothing became readable, `empty` for a non-blocking receive with no data, `not_configured` when closed or after a TCP peer EOF (which closes the socket), `data_loss` for a UDP datagram larger than the receive buffer (dropped, no callback), `invalid_config` when called from the callback, or the socket error.

The callback data pointer references the client's internal receive buffer and is valid only until the callback returns. For TCP the callback's endpoint is the peer passed to `connect()`, cached at connect time, so data already received is delivered even if the connection was reset right behind it. For UDP it is the datagram source.

## Usage Example
See `samples/castle_ext/net/client.cpp`.

```cpp
#include "castle_ext/net/client.hpp"
#include <assert.h>

int main()
{
    using namespace castle::net;
    udp_client<128U> client;
    assert(client.open() == castle::status::ok);
    castle::size_type received = 0U;
    assert(client.set_receive_callback(
        [&received](endpoint const&, uint8_t const*, castle::size_type size) { received = size; })
        == castle::status::ok);
    client.clear_receive_callback();
    assert(client.close() == castle::status::ok);
    return 0;
}
```

## Constraints & Notes
- Receive buffer and callback storage are fixed at compile time; no dynamic allocation is performed by the library.
- Callback storage is `castle::callbacks::function`; the callable must be copyable and fit the selected inline storage.
- The callback runs in place, so a `mutable` callable keeps its state between calls. It may call `set_receive_callback()`, `clear_receive_callback()` or `close()`; a replacement or clear takes effect when the callback returns, so the running callable is never destroyed under itself. It must not move or destroy the client.
- `process_once()` handles one TCP read chunk or one UDP datagram per call. It does not create a worker thread or run a background loop.
- A hard TCP receive error (such as a reset) is returned as `system_call_error` and leaves the socket open; close it from the application.
- No DNS resolver or automatic reconnect logic is included.
- The client has no internal synchronization; serialize concurrent access to one instance.
