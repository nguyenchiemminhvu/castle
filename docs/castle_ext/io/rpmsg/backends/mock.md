# Mock RPMsg Backend

## Overview
A deterministic, in-memory RPMsg backend for host samples and unit tests. It implements the same backend contract as OpenAMP while allowing tests to inject RX messages and Name Service announcements.

## Header
`#include "castle_ext/io/rpmsg/backends/mock.hpp"`

## Dependencies
[`backend.hpp`](../../backend.md), [`message.hpp`](../../message.md), [`name_service.hpp`](../../name_service.md), [`types.hpp`](../../types.md).

## Public API
`mock::basic_backend<MaxEndpoints, MaxPayload>` and alias `mock::backend`; `inject`, `announce`, `bind`, `unbind`, `fail_next_send`, `send_count`, and `last_packet`.

## Usage Example
```cpp
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

int main()
{
    using namespace castle::io::rpmsg;
    mock::backend transport;
    device<mock::backend> dev(transport);
    if (dev.initialize() != castle::status::ok) return 1;
    endpoint<mock::backend> ept;
    if (ept.create(dev, "mock.service", rpmsg_addr_any, rpmsg_addr_any, [](message_view const&) {}) != castle::status::ok) return 2;
    if (transport.bind(ept.source_address(), 3000U) != castle::status::ok) return 3;
    const uint8_t payload[] = {4U, 5U};
    if (transport.inject(ept.source_address(), 3000U, payload, sizeof(payload)) != castle::status::ok) return 4;
    if (ept.destroy() != castle::status::ok) return 5;
    return dev.shutdown() == castle::status::ok ? 0 : 6;
}
```

## Constraints & Notes
The backend stores a bounded endpoint table and one last-sent packet per instance. It does not implement actual wire framing, shared memory, or inter-processor interrupts. It is intended for deterministic API tests and examples only. Injection and announcement calls are synchronous. No allocation or console output occurs.
