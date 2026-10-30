# CoAP Umbrella Header

## Overview
Convenience header that includes all public CASTLE CoAP types, codec, UDP client, and UDP resource server APIs.

## Header
`#include "castle_ext/protocols/coap/coap.hpp"`

## Dependencies
- [CoAP Types](types.md)
- [CoAP Codec](codec.md)
- [CoAP Client](client.md)
- [CoAP Server](server.md)

## Public API
Exposes the public API of all four headers above; see their individual documentation for class templates, methods, callbacks, and configuration parameters.

## Usage Example

```cpp
#include "castle_ext/protocols/coap/coap.hpp"

int main()
{
    castle::protocols::coap::client client;
    castle::protocols::coap::server server;
    return client.is_open() || server.is_open() ? 1 : 0;
}
```

## Constraints & Notes
The umbrella itself owns no objects and adds no runtime overhead. Including it also includes the underlying CASTLE UDP networking and inline callback templates. All capacities remain compile-time configurable through the individual client/server template parameters.
