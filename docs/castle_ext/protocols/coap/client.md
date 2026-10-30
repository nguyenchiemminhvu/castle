# CoAP Client

## Overview
A polling CoAP-over-UDP client using CASTLE's existing UDP client. It retains bounded outstanding exchanges, matches replies by peer and Token, recognizes empty ACK/RST messages, acknowledges matching CON responses, suppresses recent duplicate response message IDs, and retransmits CON requests with deterministic exponential back-off.

## Header
`#include "castle_ext/protocols/coap/client.hpp"`

## Dependencies
- [Callbacks Function](../../../../include/castle/callbacks/function.hpp)
- [Core compiler macros](../../../../include/castle/core/compiler.hpp)
- [Core types](../../../../include/castle/core/types.hpp)
- [Status](../../../../include/castle/error/status.hpp)
- [Network Client](../../../../include/castle_ext/net/client.hpp)
- [CoAP Codec](codec.md)

## Public API

| API | Description |
|---|---|
| `basic_client<...>` / `client` | Client with compile-time receive buffer, exchange, option, payload, duplicate-cache, and callback capacities. |
| `open(family)`, `close()`, `is_open()` | Opens/closes the UDP socket and clears active exchanges. |
| `send_request(peer, request, handle, now_ms)` | Sends a CON or NON GET/POST/PUT/DELETE request and reserves an exchange slot. |
| `set_response_callback(callback)` | Registers `void(handle, status, message const*, endpoint const&)` callback. Nulls are used for timeouts or RST completion. |
| `cancel(handle)` | Cancels one outstanding exchange/Observe registration. |
| `process_once(now_ms, wait_ms)`, `tick(now_ms)` | Receives one datagram and/or advances timers. |
| `set_retransmission(timeout_ms, max_retransmissions)` | Configures deterministic retransmission bounds. |
| `allocate_message_id()`, `set_next_message_id(value)` | Provides an application-controlled 16-bit message ID sequence. |

## Usage Example

```cpp
#include "castle_ext/protocols/coap/client.hpp"

int main()
{
    using namespace castle::protocols::coap;
    client coap;
    if (coap.open() != castle::status::ok) return 1;
    message<> request;
    request.type = message_type::confirmable;
    request.code_value = static_cast<uint8_t>(code::get);
    request.message_id = coap.allocate_message_id();
    const uint8_t token[] = {0x10U, 0x20U};
    (void)request.set_token(token, sizeof(token));
    (void)append_uri_path(request, "well-known/core");
    client::exchange_handle exchange;
    const auto peer = castle::net::endpoint::loopback_ipv4(default_port);
    const auto sent = coap.send_request(peer, request, exchange, 0U);
    (void)coap.close();
    return sent == castle::status::ok && exchange.is_valid() ? 0 : 2;
}
```

## Constraints & Notes
The client is single-threaded and polling-based; call `process_once()` frequently with a monotonic millisecond clock. Retransmission intervals start at 2 seconds and double, with four retries by default; no random factor is injected, so this policy is deterministic rather than the RFC's randomized initial timeout. An empty ACK stops request retransmission while retaining the exchange for a separate response until the configured exchange lifetime. Observe notifications are matched by the retained token when the request includes Observe=0; full Observe registration lifecycle/serial arithmetic is not implemented. Callback storage must fit `CallbackStorageSize` and is inline.
