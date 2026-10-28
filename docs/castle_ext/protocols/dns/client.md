# DNS Client

## Overview
A fixed-buffer DNS resolver that borrows CASTLE UDP and TCP client transports. It sends standard DNS queries over UDP port 53, retries on timeout, and automatically repeats a query over TCP when the UDP response is truncated or the UDP receive layer reports a datagram larger than 512 bytes. It supports both a blocking API and a caller-driven non-blocking state machine.

## Header
`#include "castle_ext/protocols/dns/client.hpp"`

## Dependencies
- [`castle/callbacks/function.hpp`](../../../../include/castle/callbacks/function.hpp)
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/array.hpp`](../../../../include/castle/container/array.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle/utility/forward.hpp`](../../../../include/castle/utility/forward.hpp)
- [`castle/utility/move.hpp`](../../../../include/castle/utility/move.hpp)
- [`castle/container/string_view.hpp`](../../../../include/castle/container/string_view.hpp)
- [`castle_ext/net/client.hpp`](../../../../include/castle_ext/net/client.hpp)
- [`castle/callbacks/callback_dispatcher.hpp`](../../../../include/castle/callbacks/callback_dispatcher.hpp)
- [`castle_ext/protocols/dns/message.hpp`](message.md)
- [`castle_ext/protocols/dns/types.hpp`](types.md)

## Public API

| API | Description |
|---|---|
| `basic_client<UdpTransport, TcpTransport, MaxRecords, TcpMessageCapacity, CallbackStorageSize>` | Resolver borrowing compatible UDP/TCP client instances. |
| `configure(client_config)` | Sets timeout, retry count, and transaction-ID seed policy. |
| `set_callback(callback)` / `clear_callback()` | Sets or clears async completion callback `void(castle::status, response const&)`. |
| `resolve(nameserver, name, type, output, system_error)` | Blocking DNS lookup; retries UDP and transparently falls back to TCP. |
| `start_resolve(nameserver, name, type, now_ms, system_error)` | Starts a non-blocking lookup; the question name is copied internally. |
| `poll(now_ms, system_error)` | Advances one async state-machine step without waiting. Call until `busy()` is false. |
| `busy()`, `last_response()`, `last_status()` | Query asynchronous state and the latest completed result. |
| `client` | Alias for default 512-byte UDP, 2048-byte TCP, 8-record resolver. |

## Usage Example

```cpp
#include "castle_ext/protocols/dns/client.hpp"

castle::net::udp_client<512U> udp;
castle::net::tcp_client<2048U> tcp;
castle::protocols::dns::client resolver(udp, tcp);
castle::protocols::dns::response<8U> answer;
auto status = resolver.resolve(
    castle::net::endpoint::ipv4(1U, 1U, 1U, 1U, 53U),
    "example.com", castle::protocols::dns::record_type::a, answer);
```

See [`samples/castle_ext/protocols/dns/client.cpp`](../../../../samples/castle_ext/protocols/dns/client.cpp) for an online DNS + HTTP example.

## Constraints & Notes
No allocation, exceptions, RTTI, STL, or virtual dispatch are used. Each resolver supports one operation at a time and is not thread-safe. The resolver requires an IPv4/IPv6 numeric nameserver endpoint; port zero selects 53. UDP parsing is bounded to the classic 512-byte limit; the resolver uses a 513-byte receive probe so it can detect an oversized datagram even on socket targets that do not report the original datagram length. The default TCP response body capacity is 2048 bytes and can be increased with the `TcpMessageCapacity` template argument (up to 65535); answers larger than that configured capacity return `full`. `MaxRecords` independently bounds retained supported records. Async `poll()` uses an application-supplied monotonic millisecond counter, including wrap-safe elapsed-time comparison. The caller-owned transports must outlive the resolver.
