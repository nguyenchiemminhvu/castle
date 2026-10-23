# Network Endpoint

## Overview
`endpoint` is a small value type for numeric IPv4 and IPv6 addresses. It stores address octets, a host-order port, and an IPv6 scope identifier without depending on the native BSD socket ABI. Name resolution is intentionally outside this layer.

## Header
`#include "castle_ext/comm/net/endpoint.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)

## Public API
All names are in `castle::comm::net`.

| API | Description |
|---|---|
| `address_family` | `ipv4` or `ipv6` address family. |
| `endpoint()` | Creates IPv4 any-address, port zero. |
| `endpoint::ipv4(a, b, c, d, port)` | Builds an IPv4 endpoint from four octets and a host-order port. |
| `any_ipv4(port)`, `loopback_ipv4(port)` | IPv4 wildcard and loopback factories. |
| `endpoint::ipv6(bytes, port, scope_id = 0)` | Copies sixteen address bytes; a null pointer means `::`. |
| `any_ipv6(port)`, `loopback_ipv6(port)` | IPv6 wildcard and loopback factories. |
| `family()`, `port()`, `scope_id()` | Read address family, host-order port, and scope identifier. |
| `address_bytes()`, `address_size()` | Access the stored bytes; size is 4 for IPv4 and 16 for IPv6. |
| `is_any()` | True when all address bytes are zero. |
| `operator==`, `operator!=` | Compare family, port, scope, and address bytes. |

Every factory, accessor and comparison is `constexpr`, so an endpoint can be a `constexpr` value and be checked with `static_assert`.

## Usage Example
See `samples/castle_ext/comm/net/endpoint.cpp`.

```cpp
#include "castle_ext/comm/net/endpoint.hpp"
#include <assert.h>

int main()
{
    using castle::comm::net::endpoint;
    const endpoint local = endpoint::loopback_ipv4(8080U);
    assert(local.family() == castle::comm::net::address_family::ipv4);
    assert(local.port() == 8080U);
    assert(local.address_bytes()[0] == 127U);
    assert(!local.is_any());
    return 0;
}
```

## Constraints & Notes
- Fixed-size value type; no heap allocation, exceptions, RTTI, or virtual dispatch.
- Ports are always represented in host byte order. Conversion to network byte order occurs inside the socket wrapper.
- IPv6 factory input is copied immediately; it does not retain the caller's pointer.
- Only numeric addresses are represented. DNS/hostname resolution is not part of this API.
