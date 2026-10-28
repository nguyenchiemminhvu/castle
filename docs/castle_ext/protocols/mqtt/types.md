# MQTT Types

## Overview
Defines MQTT protocol versions, QoS levels, packet and reason-code identifiers, bounded MQTT 5.0 properties, CONNECT options, Will configuration, and the non-owning message view delivered to application callbacks.

## Header
`#include "castle_ext/protocols/mqtt/types.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/string_view.hpp`](../../../../include/castle/container/string_view.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)

## Public API
| Symbol | Description |
|---|---|
| `version` | Wire levels `v3_1_0`, `v3_1_1`, and `v5_0`. |
| `qos` | QoS 0, 1, and 2. |
| `packet_type` | MQTT control packet type values CONNECT through AUTH. |
| `reason_code` | Common MQTT 5.0 acknowledgement, CONNACK, and DISCONNECT reason codes. |
| `property_id` | MQTT 5.0 property identifiers. |
| `property_list<Capacity, StringCapacity, BinaryCapacity>` | Inline property storage with `add_integer`, `add_string`, `add_binary`, `add_user_property`, `find`, `clear`, and `size`. Mutations are O(1) except `find`, which is O(Capacity). |
| `message_view` | Non-owning incoming PUBLISH view: topic, payload, QoS, DUP, RETAIN, and packet identifier. |
| `will_options` | Borrowed Last Will topic/payload and QoS/RETAIN configuration. |
| `connect_options<...>` | MQTT version, client ID, clean-session/start, keep-alive, Will, credentials, and MQTT 5 property lists. |
| `to_status(reason_code)` | Maps common MQTT 5 reason codes to `castle::status`. |

## Usage Example
```cpp
#include "castle_ext/protocols/mqtt/types.hpp"

using namespace castle::protocols::mqtt;

int main()
{
    connect_options<> options;
    options.protocol_version = version::v5_0;
    options.client_id = "sensor-node-01";
    options.clean_session = false;
    options.properties.add_integer(property_id::session_expiry_interval, 3600U);
    return options.properties.count == 1U ? 0 : 1;
}
```

## Constraints & Notes
All property and option storage is fixed-capacity and allocation-free. `string_view` and payload pointers are non-owning; their storage must remain valid until CONNECT or PUBLISH encoding finishes. `message_view` payload/topic storage is valid only during the callback. The property representation covers scalar, UTF-8, binary, and User Property values without heap storage. The current client/broker implement a bounded common property subset; unsupported properties may be ignored during parsing when the call requests it.
