# Network Umbrella Header

## Overview
The networking umbrella header exposes CASTLE's numeric endpoint type, typed BSD socket wrappers, callback-driven TCP/UDP clients, and fixed-capacity TCP/UDP servers through one include.

## Header
`#include "castle_ext/net/net.hpp"`

## Dependencies
- [`endpoint.hpp`](endpoint.md)
- [`socket.hpp`](socket.md)
- [`client.hpp`](client.md)
- [`server.hpp`](server.md)

## Public API
This header adds no new types or functions. It includes the public declarations documented in:
- [`endpoint.md`](endpoint.md) — `endpoint`, `address_family`.
- [`socket.md`](socket.md) — `basic_socket`, `tcp_socket`, `udp_socket`, `socket_protocol`, `shutdown_mode`.
- [`client.md`](client.md) — `basic_client`, `tcp_client`, `udp_client`.
- [`server.md`](server.md) — `basic_server`, `tcp_server`, `udp_server`, generation-checked `client_handle`.

The internal helper [`detail/native.hpp`](detail/native.md) (BSD calls as `castle::status`) is not part of the umbrella's public surface. The in-place callback dispatcher lives in core as [`castle/callbacks/callback_dispatcher.hpp`](../../callbacks/callback_dispatcher.md).

## Usage Example
See `samples/castle_ext/net/net.cpp`.

```cpp
#include "castle_ext/net/net.hpp"
#include <assert.h>

int main()
{
    castle::net::tcp_socket socket;
    assert(!socket.is_open());
    const castle::net::endpoint peer =
        castle::net::endpoint::loopback_ipv4(80U);
    assert(peer.port() == 80U);
    return 0;
}
```

## Constraints & Notes
- The umbrella header inherits the constraints of its component headers: fixed-size storage, no heap allocation, no exceptions, no RTTI, no virtual dispatch, and no internal worker threads.
- BSD socket operations require `CASTLE_USING_POSIX_APIS`; declarations remain available on unsupported builds and return `not_configured` for system operations.
- Prefer individual component headers when compile-time inclusion cost matters.
