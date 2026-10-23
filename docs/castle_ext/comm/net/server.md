# Network Server

## Overview
`basic_server<Protocol>` provides a bounded TCP listener or UDP endpoint with a compile-time peer table. TCP connections occupy slots after `accept_client()`; UDP peers are learned from incoming datagram source endpoints. Every slot carries a generation-checked handle and an optional inline receive callback.

## Header
`#include "castle_ext/comm/net/server.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/config.hpp`](../../../../include/castle/core/config.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle/callbacks/function.hpp`](../../../../include/castle/callbacks/function.hpp)
- [`castle/utility/forward.hpp`](../../../../include/castle/utility/forward.hpp)
- [`castle/utility/move.hpp`](../../../../include/castle/utility/move.hpp)
- [`socket.hpp`](socket.md)
- [`detail/callback_dispatcher.hpp`](detail/callback_dispatcher.md)

## Public API
All names are in `castle::comm::net`.

| API | Description |
|---|---|
| `basic_server<Protocol, MaxClients = 4, ReceiveBufferSize = 256, CallbackStorageSize = inplace_storage_reserved>` | Fixed-capacity server; all capacities must be greater than zero. |
| `tcp_server<MaxClients, ReceiveBufferSize, CallbackStorageSize>` / `udp_server<...>` | TCP and UDP aliases. |
| `client_handle` | Slot index plus generation counter. Invalid/stale handles are rejected after close/reuse. |
| `open(local, backlog = 8, nonblocking = true, system_error)` | Opens and binds; TCP also enables `SO_REUSEADDR` and listens. UDP does not set `SO_REUSEADDR`, so a second server cannot silently share the port. A failed open leaves the server closed. |
| `close(system_error)`, `is_open()`, `native_handle()`, `client_count()`, `client_capacity()`, `socket()` | Lifecycle, capacity, and underlying socket access. |
| `set_default_callback(callable)`, `clear_default_callback()` | Configure the callback copied into future TCP/UDP peer slots. |
| `accept_client(out, timeout_ms = 0, system_error)` | TCP only; accepts one connection into a free slot. Capacity is checked first: a full server returns `full` at once without waiting. Otherwise it polls for `timeout_ms` (zero is non-blocking, negative waits indefinitely) and returns `empty` when nothing is pending. A pending connection that found no slot stays in the OS accept queue. |
| `set_client_callback(handle, callable)`, `clear_client_callback(handle)` | Replace or clear a slot's callback. Invalid/stale handles return `not_found`. |
| `process_client(handle, timeout_ms = 0, system_error)` | TCP only; waits for and dispatches one stream receive. EOF and fatal receive errors such as `ECONNRESET` close the connection and free the slot (EOF returns `not_configured`, an error `system_call_error`). |
| `process_datagram(out, timeout_ms = 0, system_error, source = nullptr)` | UDP only; receives one datagram, creates/reuses its source peer slot, and dispatches the callback. `full` means a new peer arrived at capacity: the datagram is dropped, no callback runs, and `source` (if given) reports who sent it. A datagram larger than the receive buffer is dropped as `data_loss`. |
| `send_some(handle, data, size, bytes_sent, system_error)` | Sends to a TCP slot or to the UDP endpoint represented by a peer handle. |
| `send_all(handle, data, size, bytes_sent, system_error)` | TCP only. |
| `client_endpoint(handle, out)` | Returns the endpoint associated with the active slot. |
| `client_at(index)` | Handle of the active client in slot `index` (`0 .. client_capacity()`), or an invalid handle. Iterate it to enumerate clients or pick a UDP peer to evict. |
| `client_native_handle(handle)` | Descriptor to poll for one client, or `-1` for a stale handle. TCP clients have their own; UDP peers share the listener's. Register these with `castle::io::fd::fd_event_manager` to wait on every client with one `poll()`. |
| `close_client(handle, system_error)` | Closes a TCP connection or removes a remembered UDP peer. |

Receive callbacks use signature `void(client_handle, endpoint const&, uint8_t const*, size_type)`. A callback is called synchronously in the processing call and the data pointer is valid only during that callback. UDP peers remain in the table until explicitly closed or the server closes; there is no idle timeout/eviction policy, so applications that expect more peers than `MaxClients` evict through `client_at()` and `close_client()` when `process_datagram()` reports `full`.

## Usage Example
See `samples/castle_ext/comm/net/server.cpp`.

```cpp
#include "castle_ext/comm/net/server.hpp"
#include <assert.h>

int main()
{
    using namespace castle::comm::net;
    tcp_server<4U, 256U> server;
    assert(server.set_default_callback(
        [](tcp_server<4U, 256U>::client_handle, endpoint const&, uint8_t const*, castle::size_type) {})
        == castle::status::ok);
    assert(server.open(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    assert(server.client_capacity() == 4U);
    assert(server.close() == castle::status::ok);
    return 0;
}
```

## Constraints & Notes
- Peer table, per-peer sockets, the receive buffer, and callback objects are members of the server, so `sizeof(server)` is its entire memory cost; nothing is placed on the stack. No heap allocation, exceptions, RTTI, virtual dispatch, or threads are used by the library.
- `MaxClients` bounds simultaneous TCP connections or distinct remembered UDP source endpoints.
- Handles include a generation counter, preventing stale handles from accidentally identifying a slot after it is reused.
- Callbacks run in place, so a `mutable` callable keeps its state between calls. A callback may replace or clear its own callback, close its own peer (or the whole server), send, accept, and manage other peers; those changes to the running slot take effect when it returns. It must not call `process_client()`/`process_datagram()` (rejected with `invalid_config`, because the receive buffer is in use) or destroy the server.
- The server is not internally synchronized. The application must serialize calls that mutate/iterate the slot table or operate on the same socket.
- Each call to `accept_client()`, `process_client()`, or `process_datagram()` handles at most one connection/datagram. Applications own their event-loop policy; use `client_native_handle()` to build a multiplexed loop.
- UDP peer entries are keyed by source address, port, and IPv6 scope. When the table is full, a datagram from an unknown peer has already been received before `full` is returned; TCP instead leaves a pending connection in the OS accept queue.
