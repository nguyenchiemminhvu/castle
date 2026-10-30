# CoAP Server

## Overview
A bounded, polling CoAP-over-UDP resource server built on CASTLE's UDP server peer table. It performs exact hierarchical Uri-Path dispatch, generates piggybacked ACK or NON responses, suppresses duplicate requests for the RFC exchange lifetime, and supports manually generated separate responses with bounded CON retransmission.

## Header
`#include "castle_ext/protocols/coap/server.hpp"`

## Dependencies
- [Callbacks Function](../../../../include/castle/callbacks/function.hpp)
- [Core compiler macros](../../../../include/castle/core/compiler.hpp)
- [Core types](../../../../include/castle/core/types.hpp)
- [Status](../../../../include/castle/error/status.hpp)
- [Network Server](../../../../include/castle_ext/net/server.hpp)
- [CoAP Codec](codec.md)

## Public API

| API | Description |
|---|---|
| `basic_server<...>` / `server` | Server with compile-time peer/resource/recent-ID/retransmit/path/buffer capacities. |
| `open(endpoint)`, `close()`, `is_open()` | Binds/closes the UDP server socket. |
| `register_resource(path, methods, callback)` | Registers an exact URI path and allowed method bitmask; handler receives request and mutable response. |
| `unregister_resource(path)` | Removes a registered resource. |
| `set_resource_max_age(path, seconds)` | Sets the default Max-Age option added to that resource's responses unless the handler supplies one. |
| `configure_rate_limit(maximum_requests, window_ms)`, `clear_rate_limit()` | Enables/disables a per-source fixed-window limiter; rejected requests receive 5.03 with Max-Age indicating the retry delay. |
| `process_once(now_ms, timeout_ms)` | Processes at most one UDP datagram and advances timers. |
| `send_empty_ack(peer, message_id)` | Emits an empty ACK. |
| `send_separate_response(peer, request, response, now_ms, confirmable)` | Sends empty ACK followed by a separate CON/NON response; CON response is retransmitted until ACK/RST or expiry. |
| `tick(now_ms)` | Drives pending server CON response retransmissions. |
| `client_count()`, `resource_count()`, `transport()` | Bounded runtime state and access to underlying UDP transport. |

## Usage Example

```cpp
#include "castle_ext/protocols/coap/server.hpp"

int main()
{
    using namespace castle::protocols::coap;
    server coap;
    if (coap.register_resource("sensors/temp", allow_get,
        [](message<> const&, message<>& response) {
            response.code_value = static_cast<uint8_t>(code::content);
            const char value[] = "22.5";
            (void)response.set_payload(reinterpret_cast<uint8_t const*>(value), sizeof(value) - 1U);
        }) != castle::status::ok) return 1;
    // Bind to a chosen local UDP endpoint and call process_once() from the event loop.
    return coap.resource_count() == 1U ? 0 : 2;
}
```

## Constraints & Notes
Resource callbacks execute synchronously in the UDP receive callback; the server is not internally synchronized and must not recursively call `process_once()`. Peer slots are bounded by the underlying UDP server and are not aged out automatically. Duplicate detection keys on source endpoint and Message ID; cached replies occupy `MaxRecentMessages * ReceiveBufferSize` bytes. The optional limiter stores one fixed-window counter per configured client slot and is disabled by default. Resource responses carry a default Max-Age of 60 seconds (RFC default) unless the handler or resource configuration supplies another value. Separate CON response retransmission is implemented, but server-side automatic Observe subscriptions/notifications, Block-Wise assembly, Max-Age representation caching, rate limiting, and dynamic discovery are not. Unknown critical options in a CON request produce 4.02 Bad Option; malformed CON datagrams are reset where a usable Message ID is available.
