# MQTT Broker

## Overview
A standalone, single-process MQTT broker over `castle::net::tcp_server`. It accepts a compile-time maximum number of TCP clients, parses CONNECT and the core control packets, manages subscriptions and retained messages, routes publications using MQTT Topic Filter matching, and executes QoS 1/2 acknowledgement flows.

## Header
`#include "castle_ext/protocols/mqtt/broker.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/string_view.hpp`](../../../../include/castle/container/string_view.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle_ext/net/server.hpp`](../../../../include/castle_ext/net/server.hpp)
- [`codec.hpp`](codec.md)
- [`topic.hpp`](topic.md)
- [`types.hpp`](types.md)

## Public API
| API | Description |
|---|---|
| `basic_broker<MaxClients, ReceiveBufferSize, MaxPacketSize, MaxSubscriptions, MaxRetained, MaxOutstanding, MaxTopicLength, MaxPayloadSize, MaxProperties, ...>` | Configures all client, stream, packet, subscription, retained-message, QoS, topic, payload, and property capacities. |
| `open(endpoint, backlog)` | Opens a non-blocking TCP listener; defaults to IPv4 any-address on port 1883. |
| `close()` | Closes the listener and all accepted connections. |
| `set_authentication(username, password)` | Configures optional fixed-buffer username/password checking. Empty username disables authentication. |
| `poll(now_ms, accept_timeout_ms)` | Accepts one connection and processes one receive chunk per active client; enforces keep-alive deadlines. |
| `client_count`, `subscription_count`, `retained_count` | Reports active and retained table counts. |
| `transport()` | Exposes the underlying fixed-capacity TCP server for advanced integration. |
| `broker` | Default-capacity alias for `basic_broker<>`. |

## Usage Example
```cpp
#include "castle_ext/protocols/mqtt/broker.hpp"

int main()
{
    castle::protocols::mqtt::broker server;
    if (server.open(castle::net::endpoint::any_ipv4(1883U)) != castle::status::ok) { return 1; }
    (void)server.poll(0U); // Repeat from the application's main loop with monotonic milliseconds.
    return server.close() == castle::status::ok ? 0 : 2;
}
```

## Constraints & Notes
The broker is polling-based, single-threaded, and not internally synchronized. All operations must be serialized by the owner. Client/session/subscription/retained/QoS buffers are fixed-capacity members; exhaustion returns a CASTLE status or rejects the packet. Session subscriptions and retained messages are stored in RAM only. Clean Session/Clean Start clears subscriptions; non-clean sessions may retain subscription records in RAM (MQTT 5 retention observes Session Expiry Interval). Persistent offline delivery of queued QoS messages is not yet implemented. MQTT 3.1.0, 3.1.1, and bounded MQTT 5.0 core packets are accepted. AUTH, shared subscriptions, advanced MQTT 5 properties, disk persistence, TLS, WebSocket, and clustering are outside this implementation scope.
