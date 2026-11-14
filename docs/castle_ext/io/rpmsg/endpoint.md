# RPMsg Endpoint

## Overview
A move-only logical endpoint handle that registers an RX callback and sends to the endpoint's current or explicitly selected peer address.

## Header
`#include "castle_ext/io/rpmsg/endpoint.hpp"`

## Dependencies
[`types.hpp`](../types.md), [`message.hpp`](../message.md), [`backend.hpp`](../backend.md), `castle/callbacks/function.hpp`, `castle/core/status.hpp`.

## Public API
`basic_endpoint<Backend, Options>` and alias `endpoint<Backend, Options>`; `create`, `destroy`, `send`, `try_send`, `send_to`/`sendto`, `try_send_to`/`try_sendto`, `send_offchannel`, `try_send_offchannel`, `acquire_tx_buffer`, `send_nocopy`, and `release_tx_buffer`.

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
    if (ept.create(dev, "sample.service", rpmsg_addr_any, rpmsg_addr_any,
                   [](message_view const&) {}) != castle::status::ok) return 2;
    if (transport.bind(ept.source_address(), 2048U) != castle::status::ok) return 3;
    const char text[] = "hi";
    if (ept.try_send(text, 2U) != castle::status::ok) return 4;
    if (ept.destroy() != castle::status::ok) return 5;
    return dev.shutdown() == castle::status::ok ? 0 : 6;
}
```

## Constraints & Notes
Endpoints are move-only RAII handles, refer to one slot/generation in their parent device, and must not outlive that device. Destruction unregisters an active endpoint. Explicit local and remote addresses below 1024 are rejected; `RPMSG_ADDR_ANY` requests local allocation or defers remote binding. Explicit off-channel sends still require a valid local address. Send payloads are checked against `Options::max_payload_size`. The RX view is callback-scoped. TX buffers must be sent or released by the endpoint that acquired them before that endpoint/device is destroyed. Endpoint operations are not internally synchronized.
