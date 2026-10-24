# HTTP Client Adapters

## Overview
Provides template-based, caller-polled HTTP/1.1 and HTTP/2 client adapters over a CASTLE TCP-client-compatible transport. It preserves explicit transport ownership, avoids virtual dispatch, forwards nonblocking send/receive and timeout statuses, and dispatches fixed-storage response/body callbacks.

## Header
`#include "castle_ext/comm/http/client.hpp"`

## Dependencies
- [HTTP/1.1 codec](http1.md)
- [HTTP/2 session](http2.md)
- [HTTP bounded headers](headers.md)
- [HTTP shared types](types.md)
- [CASTLE callbacks](../../../../include/castle/callbacks/function.hpp)
- [CASTLE compiler annotations](../../../../include/castle/core/compiler.hpp)
- [CASTLE core types](../../../../include/castle/core/types.hpp)
- [CASTLE status](../../../../include/castle/error/status.hpp)
- [Network endpoint](../net/endpoint.md)

## Public API
- `http1_client<Transport, ...>`: serializes one request at a time, drives connect/send/receive with `poll`, supports sequential keep-alive reuse, and exposes the completed response.
- `http2_client<Transport, ...>`: starts a prior-knowledge HTTP/2 session, opens concurrent streams, and pumps session output/input through the transport.
- `set_response_callback(callback)`: callback signature is `void(castle::status, response_view const&)`.
- `set_body_callback(callback)`: receives body chunks while parsing.
- `connect(endpoint, ...)`, `request(...)` / `open_stream(...)`, `poll(timeout_ms)`, `close()`: client lifecycle.
- `http1_policy`, `http2_policy`, `protocol_policy_traits`, `client_type`, `basic_http_client`: compile-time policy selection; no virtual functions.

## Usage Example
```cpp
#include "castle_ext/comm/http/client.hpp"
#include "castle_ext/comm/net/client.hpp"

int main()
{
    using namespace castle;
    using namespace castle::comm;
    net::tcp_client<512U> tcp;
    http::http1_client<decltype(tcp)> client(tcp);
    bool completed = false;
    if (client.set_response_callback([&completed](status result, http::response_view const& response)
        { completed = result == status::ok && response.status_code == 200U; }) != status::ok) { return 1; }
    const net::endpoint remote = net::endpoint::ipv4(192U, 0U, 2U, 10U, 80U);
    status result = client.connect(remote, "example.test:80", 0);
    if (result != status::ok && result != status::time_out) { return 2; }
    for (size_type i = 0U; !client.connected() && i < 1000U; ++i)
    {
        result = client.poll(50);
        if (result != status::ok && result != status::time_out) { return 3; }
    }
    if (!client.connected()) { return 4; }
    http::http1_client<decltype(tcp)>::headers_type headers;
    if (client.request(http::method::get, "/health", headers) != status::ok) { return 5; }
    for (size_type i = 0U; i < 1000U && client.busy(); ++i)
    {
        result = client.poll(50);
        if (result != status::ok && result != status::time_out && result != status::empty && result != status::full) { return 6; }
    }
    return completed ? 0 : 7;
}
```

## Constraints & Notes
`Transport` is passed by reference and must outlive the client. The adapter expects the CASTLE TCP client's nonblocking `connect`, `send_some`, `receive_some`, `close`, `socket().finish_connect`, and `socket().wait_readable` interface. The adapter is intentionally not tied to OS descriptor headers, allowing protocol-only mock testing; include the platform's CASTLE FD backend when instantiating `net::tcp_client`. HTTP/1.1 is strictly one in-flight request per connection; HTTP/2 supports compile-time-bounded streams. The HTTP/2 request-body view must outlive transmission. Call `poll` repeatedly to advance operations; returned timeout/backpressure statuses are not hidden. No TLS/HTTPS.
