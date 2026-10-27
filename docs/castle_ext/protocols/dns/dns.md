# DNS Umbrella Header

## Overview
Convenience header that includes all public CASTLE DNS types, message codec, and client APIs.

## Header
`#include "castle_ext/protocols/dns/dns.hpp"`

## Dependencies
- [`castle_ext/protocols/dns/types.hpp`](types.md)
- [`castle_ext/protocols/dns/message.hpp`](message.md)
- [`castle_ext/protocols/dns/client.hpp`](client.md)

## Public API
This header adds no declarations. It exposes the APIs documented by the three included headers.

## Usage Example

```cpp
#include "castle_ext/protocols/dns/dns.hpp"

castle::protocols::dns::response<8U> answer;
```

See [`samples/castle_ext/protocols/dns/dns.cpp`](../../../../samples/castle_ext/protocols/dns/dns.cpp).

## Constraints & Notes
The umbrella header inherits the fixed-capacity, allocation-free constraints of each included component. Include the individual header instead when a translation unit needs to minimize compile-time dependencies.
