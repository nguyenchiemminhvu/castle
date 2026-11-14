# HTTP/1.1 Server

## Overview

Provides a fixed-capacity, zero-allocation HTTP/1.0 and HTTP/1.1 server over CASTLE's TCP server. It owns bounded request/response models, parses `Content-Length` and chunked request bodies, supports persistent connections and sequential pipelining, and dispatches exact, prefix, and `{parameter}` routes. Handlers execute synchronously on the thread calling `poll()`.

## Header

`#include "castle_ext/protocols/http/server.hpp"`

## Dependencies

- [HTTP shared types](types.md)
- [Fixed-capacity HTTP headers](headers.md)
- [`castle::callbacks::function`](../../../../include/castle/callbacks/function.hpp)
- [`castle::error_handler`](../../../../include/castle/core/error_handler.hpp)
- [`castle::net::tcp_server`](../../../../include/castle_ext/net/server.hpp)
- [`castle::net::endpoint`](../../../../include/castle_ext/net/endpoint.hpp)
- [`castle::status`](../../../../include/castle/core/status.hpp)

## Public API

| Component | Description |
|---|---|
| `HttpMethod` | Drogon-style method names: `GET`, `POST`, `PUT`, `DELETE`, `HEAD`, `OPTIONS`, and `PATCH`. |
| `status_code` | Named response status constants, including common 1xx–5xx values, 417, 431, and 505. |
| `basic_http_request<...>` / `HttpRequest` | Owning parsed method, target, normalized path, query, headers, body, and captured path parameters. `query_param()` and `path_param()` return non-owning views. |
| `basic_http_response<...>` / `HttpResponse` | Fixed-buffer response with `set_status()`, `set_header()`, `set_body()`, `set_json()`, and `set_file()`. `set_file()` copies caller-supplied static bytes; it does not access a filesystem. |
| `basic_http_server<...>` / `HttpServer` | Fixed-capacity TCP server and router. Template parameters configure connections, receive chunks, requests per connection, routes, headers, URI/body/request sizes, and callback storage. |

The server template parameters, in order, are `MaxConnections`, `ReceiveBufferSize`, `MaxRequestsPerConnection`, `MaxRoutes`, `MaxHeaders`, `MaxHeaderNameSize`, `MaxHeaderValueSize`, `MaxUriSize`, `MaxBodySize`, `MaxRequestSize`, `MaxHeaderBytes`, `MaxRoutePathSize`, `MaxPathSegments`, and `HandlerStorageSize`. The defaults are listed in `server.hpp`.
| `register_handler(path, handler)` | Registers a handler for any recognized method on an exact or parameterized path. |
| `register_handler(method, path, handler)` | Registers a method-specific exact/parameterized route. `HttpMethod` and the existing lowercase `method` enum are accepted. |
| `register_prefix_handler(...)` | Registers exact-prefix plus descendant-path matching. |
| `set_not_found_handler(handler)` | Replaces the built-in 404 response with an application fallback. |
| `start(endpoint, backlog)` / `poll(timeout)` / `stop()` | Opens the listener, accepts and polls peers, and releases all sockets. A port of zero requests an ephemeral port; query it with `local_endpoint()`. |
| `last_registration_status()` | Reports the result of the most recent route/fallback registration; registration methods return `*this` for chaining. |

Routing uses a fixed linear scan: O(number of registered routes × path length). Header lookup is O(header count). No route, request, or response operation allocates memory.

## Usage Example

```cpp
#include "castle_ext/protocols/http/server.hpp"

using namespace castle::protocols::http;

int main()
{
    HttpServer server;
    server.register_handler(HttpMethod::GET, "/devices/{id}/state",
        [](HttpRequest const& request, HttpResponse& response)
        {
            response.set_json(request.path_param("id"));
        });
    if (server.last_registration_status() != castle::status::ok)
    {
        return 1;
    }
    if (server.start(castle::net::endpoint::loopback_ipv4(8080U)) != castle::status::ok)
    {
        return 2;
    }
    while (true)
    {
        (void)server.poll(10);
    }
}
```

The online samples are `samples/castle_ext/protocols/http/server.cpp` and `server_client.cpp`. The server sample listens on `127.0.0.1:8080` by default and accepts an optional port argument; it runs until Ctrl+C. In another terminal, run `python3 samples/castle_ext/protocols/http/py_http_client/client.py`. When using a non-default port, start the server with that port and set `CASTLE_HTTP_PORT` for the Python script. The CASTLE C++ client sample also accepts the port as its first argument.

## Constraints & Notes

- All storage is inline, including the reusable response object and response serialization buffer (kept out of the polling stack frame). `MaxRequestSize` includes headers, body bytes, and chunk framing; `MaxBodySize` applies to the decoded request and response bodies.
- Only HTTP/1.0 and HTTP/1.1 are implemented. HTTP/2 preface/prior-knowledge traffic is rejected with 505; HTTP/2 framing, HPACK server encoding, and multiplexed streams are not implemented by this header.
- `Content-Length` and a single `Transfer-Encoding: chunked` are supported for requests. Conflicting framing, duplicate `Content-Length`, unsupported transfer codings, malformed request lines/headers/chunks, and exceeded capacities are rejected. Buffered responses use a generated `Content-Length`; application-supplied `Transfer-Encoding` is removed because response chunk streaming is not implemented.
- Persistent connections are enabled by HTTP/1.1 default semantics and bounded by `MaxRequestsPerConnection`. HTTP/1.0 requires `Connection: keep-alive`. Pipelined requests are processed sequentially, one at a time, in wire order. Absolute-form `http://` and `https://` targets are normalized to paths; `OPTIONS *` returns a server-wide `Allow` response. Query values and path captures are returned as received and are not percent-decoded.
- Accepted sockets are blocking to make bounded `send_all()` response writes deterministic. `poll()` is non-blocking for active peers; the application should yield/sleep between repeated calls when idle.
- Idle-timeout scheduling, connection rate limiting, deferred/asynchronous handlers, response chunk streaming, middleware chains, TLS, filesystem access, and HTTP/2 are not provided in this initial server implementation.
- `set_file()` serves only caller-owned in-memory content copied into the response buffer; no OS file API is used.
- Register routes before `start()`. The server is not internally synchronized: `start()`, `poll()`, `stop()`, and handlers must be serialized on one thread. Request/header/body views are valid only for the synchronous handler call.
- Error reporting uses CASTLE's configured `castle::error_handler::error()` callback when `CASTLE_LOG_ERRORS` or `CASTLE_USE_ASSERT_FUNCTION` is enabled. In builds without either error mode, protocol failures remain observable through their HTTP response and connection behavior but the global logger is compiled out.
- No STL dependency, heap allocation, exceptions, RTTI, or virtual dispatch is introduced by this header.
