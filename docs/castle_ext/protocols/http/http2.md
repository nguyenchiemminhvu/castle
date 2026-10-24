# HTTP/2 Framing and Session

## Overview
Implements an incremental HTTP/2 frame parser and a static-memory client session for prior-knowledge cleartext HTTP/2 (h2c). The session sends the mandatory connection preface and SETTINGS, builds HEADERS/CONTINUATION/DATA frames, decodes bounded HPACK, tracks concurrent streams, handles control frames, and maintains connection/stream flow-control windows.

## Header
`#include "castle_ext/protocols/http/http2.hpp"`

## Dependencies
- [HPACK codec](hpack.md)
- [HTTP bounded headers](headers.md)
- [HTTP shared types](types.md)
- [CASTLE callbacks](../../../../include/castle/callbacks/function.hpp)
- [CASTLE compiler annotations](../../../../include/castle/core/compiler.hpp)
- [CASTLE core types](../../../../include/castle/core/types.hpp)
- [CASTLE status](../../../../include/castle/error/status.hpp)

## Public API
- `http2_frame_parser<MaxFrameSize>::feed(data, size, callback, consumed)`: incrementally parses bounded 9-byte frame headers and payloads.
- `http2_session<MaxConcurrentStreams, MaxHeaderBlockSize, MaxFrameSize, MaxBodySize, ..., MaxDynamicTableBytes, ...>::start()`: queues the HTTP/2 client preface and initial SETTINGS.
- `open_stream(method, path, authority, headers, body, stream_id, custom_method)`: opens a client-initiated odd-numbered stream and queues request headers/body.
- `feed(data, size, peer_closed)`: processes received server frames.
- `pending_output`, `pending_output_size`, `output_size`, `consume_output`: expose and retire contiguous output bytes for the TCP writer.
- `grant_connection_window`, `grant_stream_window`: queue WINDOW_UPDATE and add receive credit.
- `response`, `release_stream`, `active_stream_count`, `goaway_received`, `last_error`: stream/session inspection and lifecycle.
- `set_response_callback`, `set_body_callback`: install fixed-storage callbacks.

## Usage Example
```cpp
#include "castle_ext/protocols/http/http2.hpp"

int main()
{
    using namespace castle::protocols::http;
    http2_session<2U, 512U, 16384U, 256U> session;
    if (session.start() != castle::status::ok) { return 1; }
    http2_session<2U, 512U, 16384U, 256U>::headers_type headers;
    uint32_t stream_id = 0U;
    if (session.open_stream(method::get, "/", "example.test", headers, string_view(), stream_id) != castle::status::ok) { return 2; }
    return stream_id == 1U && session.output_size() > 24U ? 0 : 3;
}
```

## Constraints & Notes
The parser requires `MaxFrameSize >= 16,384`; all payload, stream, header-block, body, HPACK, and output storage is fixed at compile time. `MaxConcurrentStreams` is a hard local upper bound, additionally limited by the peer's SETTINGS. `MaxDynamicTableBytes = 0` disables dynamic HPACK memory and is advertised to the peer. Request body views must remain valid until the session has queued all DATA frames. Retained response bodies are capped by `MaxBodySize`; a body callback is required to continue past that capacity. Stream records remain allocated until `release_stream`, even after completion. Flow-control credit is explicitly managed by the caller with WINDOW_UPDATE helpers. Server push is rejected; unknown extension frames are ignored. Plain TCP prior knowledge only: no HTTP Upgrade, ALPN, TLS, or HTTP/3.
