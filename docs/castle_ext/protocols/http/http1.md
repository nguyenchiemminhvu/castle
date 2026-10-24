# HTTP/1.1 Request and Response Codec

## Overview
Contains a fixed-buffer HTTP/1.x request serializer and a byte-at-a-time/incremental response parser. The parser handles status/header lines, explicit Content-Length, close-delimited bodies, and an opt-in bounded chunked decoder.

## Header
`#include "castle_ext/protocols/http/http1.hpp"`

## Dependencies
- [HTTP bounded headers](headers.md)
- [HTTP shared types](types.md)
- [CASTLE callbacks](../../../../include/castle/callbacks/function.hpp)
- [CASTLE compiler annotations](../../../../include/castle/core/compiler.hpp)
- [CASTLE core types](../../../../include/castle/core/types.hpp)
- [CASTLE status](../../../../include/castle/error/status.hpp)

## Public API
- `http1_request<Capacity, MaxHeaders, MaxNameSize, MaxValueSize>::build(...)`: serializes a request, adding Host, Content-Length when required, and Connection when absent.
- `data()`, `size()`: inspect the serialized request bytes.
- `http1_response_parser<MaxBodySize, ..., EnableChunked>::feed(data, size, peer_closed, consumed)`: incrementally parses one response and reports bytes consumed.
- `set_body_callback(callback)`: receives body slices as they are consumed; when retained body storage fills, it permits continued streaming.
- `reset(head_response)`, `complete`, `failed`, `last_error`, `response`: parser lifecycle and response accessors.

## Usage Example
```cpp
#include "castle_ext/protocols/http/http1.hpp"

int main()
{
    using namespace castle::protocols::http;
    basic_headers<4U, 32U, 64U> request_headers;
    http1_request<512U> request;
    if (request.build(method::get, "/status", "device.local:80", request_headers) != castle::status::ok) { return 1; }
    http1_response_parser<128U> parser;
    const char response[] = "HTTP/1.1 204 No Content\r\n\r\n";
    if (parser.feed(response, sizeof(response) - 1U) != castle::status::ok) { return 2; }
    return parser.complete() && parser.response().status_code == 204U ? 0 : 3;
}
```

## Constraints & Notes
Request and parser memory is inline and compile-time bounded. The parser is deterministic and consumes each byte once; it does not perform large-buffer backtracking. Chunked decoding is disabled by default and reports `error_code::chunked_not_supported`; enable `EnableChunked` for the bounded extension, which also validates and retains permitted trailer fields. Informational responses are skipped until a final response arrives; `101 Switching Protocols` is rejected because upgrades are out of scope. Bodies larger than `MaxBodySize` require a registered body callback or fail with `body_limit`. Close-delimited bodies complete only when the peer closes. One parser instance handles one response at a time; connection reuse is coordinated by the client adapter. No TLS, gzip/deflate, heap, exceptions, or STL.
