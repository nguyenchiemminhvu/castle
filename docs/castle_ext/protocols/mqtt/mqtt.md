# MQTT Module

## Overview
Umbrella include for CASTLE's allocation-free MQTT client, broker, packet codec, topic utilities, and protocol types. Use it when an application needs the complete public MQTT API from one include.

## Header
`#include "castle_ext/protocols/mqtt/mqtt.hpp"`

## Dependencies
- [`types.hpp`](types.md)
- [`topic.hpp`](topic.md)
- [`codec.hpp`](codec.md)
- [`client.hpp`](client.md)
- [`broker.hpp`](broker.md)

## Public API
The umbrella exposes all symbols described by the five component documents above: `client` / `basic_client`, `broker` / `basic_broker`, packet framing and property codecs, topic validation/matching, MQTT versions, QoS, properties, CONNECT options, and message views.

## Usage Example
```cpp
#include "castle_ext/protocols/mqtt/mqtt.hpp"

int main()
{
    castle::protocols::mqtt::client subscriber;
    castle::protocols::mqtt::broker local_broker;
    return (!subscriber.is_connected() && local_broker.client_count() == 0U) ? 0 : 1;
}
```

## Constraints & Notes
The module remains header-only and allocation-free. Actual memory use depends on the selected template capacities and object instances. Include component headers instead of the umbrella when minimizing compile dependencies is important. See the client and broker documents for supported protocol subsets and operational limitations.
