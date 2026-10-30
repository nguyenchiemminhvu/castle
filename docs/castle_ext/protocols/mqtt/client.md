# MQTT Client

## Overview
A polling-based MQTT client over `castle::net::tcp_client`. It builds CONNECT, PUBLISH, SUBSCRIBE, UNSUBSCRIBE, PINGREQ, and DISCONNECT packets, parses inbound packets from an arbitrarily chunked TCP stream, dispatches received messages, and maintains bounded QoS 1/2 handshakes.

## Header
`#include "castle_ext/protocols/mqtt/client.hpp"`

## Dependencies
- [`castle/callbacks/function.hpp`](../../../../include/castle/callbacks/function.hpp)
- [`castle/callbacks/callback_dispatcher.hpp`](../../../../include/castle/callbacks/callback_dispatcher.hpp)
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/string_view.hpp`](../../../../include/castle/container/string_view.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle_ext/net/client.hpp`](../../../../include/castle_ext/net/client.hpp)
- [`codec.hpp`](codec.md)
- [`topic.hpp`](topic.md)
- [`types.hpp`](types.md)

## Public API
| API | Description |
|---|---|
| `basic_client<ReceiveBufferSize, MaxPacketSize, MaxOutstanding, MaxTopicLength, MaxPayloadSize, MaxProperties, ...>` | Configures all receive, packet, QoS, topic, payload, and property capacities at compile time. |
| `set_message_callback(callback)` / `clear_message_callback()` | Installs or clears the synchronous message-arrived callback using inline `castle::callbacks::function` storage. Replacing or clearing the callback from inside itself takes effect when it returns (via `callback_dispatcher`). |
| `connect(endpoint, connect_options, timeout_ms)` | Opens TCP, sends CONNECT, and waits for CONNACK. The socket is numeric IPv4/IPv6; MQTT convention is port 1883. |
| `publish(topic, payload, size, qos, retain)` | Sends PUBLISH; QoS 1/2 reserve a bounded packet-ID state entry. |
| `subscribe(filter, qos, packet_id)` / array overload | Sends one or more Topic Filters in SUBSCRIBE. |
| `unsubscribe(filter, packet_id)` / array overload | Sends one or more filters in UNSUBSCRIBE. |
| `poll(now_ms, timeout_ms)` | Processes one receive event and services keep-alive timers. The caller supplies monotonic milliseconds. |
| `disconnect(reason)` / `force_close()` | Sends MQTT DISCONNECT when requested, or closes TCP immediately. |
| `is_connected`, `is_open`, `session_present`, `last_error`, `last_acknowledged_packet_id`, `transport` | State and transport accessors. |
| `client` | Default-capacity alias for `basic_client<>`. |

## Usage Example
```cpp
#include "castle_ext/protocols/mqtt/client.hpp"

int main()
{
    using namespace castle::protocols::mqtt;
    client mqtt;
    connect_options<> options;
    options.client_id = "embedded-node";
    if (mqtt.connect(castle::net::endpoint::loopback_ipv4(1883U), options) != castle::status::ok) { return 1; }
    uint16_t subscription = 0U;
    if (mqtt.subscribe("sensors/+/temperature", qos::at_least_once, subscription) != castle::status::ok) { return 2; }
    (void)mqtt.poll(0U, 10);
    return mqtt.disconnect() == castle::status::ok ? 0 : 3;
}
```

## Constraints & Notes
No heap allocation, exceptions, RTTI, or virtual functions are used by the MQTT layer. Every packet, reassembly buffer, QoS table, topic, and retained payload has compile-time capacity. Client callbacks execute synchronously while `poll()` processes a network receive and must not re-enter `poll()`. A message callback may replace or clear itself; the change takes effect when it returns. Message views are valid only during the callback. `poll()` needs a monotonically increasing millisecond value from the application; no hidden clock is read. The implementation supports MQTT 3.1.0, 3.1.1, and a bounded MQTT 5.0 core (CONNECT/CONNACK, core reason/property parsing, PUBLISH acknowledgements, and DISCONNECT). MQTT 5 AUTH exchanges and the full advanced property/flow-control set are not implemented. Reconnection retains the client-side QoS table only when the broker reports Session Present; persistent broker-side pending delivery queues are not yet implemented.
