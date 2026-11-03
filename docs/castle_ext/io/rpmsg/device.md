# RPMsg Device

## Overview
Fixed-capacity device facade that owns endpoint/service tables and connects application callbacks to a platform backend.

## Header
`#include "castle_ext/io/rpmsg/device.hpp"`

## Dependencies
[`types.hpp`](../types.md), [`message.hpp`](../message.md), [`name_service.hpp`](../name_service.md), [`backend.hpp`](../backend.md), [`endpoint.hpp`](../endpoint.md), `castle/callbacks/function.hpp`, `castle/container/string.hpp`.

## Public API
`basic_device<Backend, Options>` and alias `device<Backend, Options>`; `initialize`, `shutdown`, `poll`, `set_name_service_callback` (unclaimed CREATE and device-level DESTROY events), `find_service`, `process_name_service_event`, `endpoint_count`, `service_count`, and `backend`.

## Usage Example
```cpp
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

int main()
{
    using namespace castle::io::rpmsg;
    mock::backend transport;
    device<mock::backend> dev(transport);
    if (dev.set_name_service_callback([](name_service_event const&) {}) != castle::status::ok) return 1;
    if (dev.initialize() != castle::status::ok) return 2;
    if (transport.announce("log.service", 2200U) != castle::status::ok) return 3;
    address remote = rpmsg_addr_any;
    if (dev.find_service("log.service", remote) != castle::status::ok || remote != 2200U) return 4;
    return dev.shutdown() == castle::status::ok ? 0 : 5;
}
```

## Constraints & Notes
Storage is embedded in the device object: `MaxEndpoints` endpoint slots, `MaxChannels` cached NS records, fixed strings, and callback storage. Runtime buffer dimensions are capped by the selected `Options` type. Operations scan those arrays linearly (O(MaxEndpoints) or O(MaxChannels)). The backend and device must outlive all endpoint handles. The device destructor performs best-effort shutdown. The facade is not thread-safe; serialize endpoint changes and callback activity at the application/platform level. Name Service records are keyed by service name and remote address. For backends that dispatch unmatched events, a CREATE binds only endpoints not already bound; a DESTROY removes only that remote address. OpenAMP performs its own name/address matching for registered endpoints and calls the endpoint unbind handler on peer destruction. Valid CREATE/DESTROY events delivered to the facade are passed to the configured callback when `Options::auto_bind` is enabled, even when the bounded cache is full or the record was not previously cached. `auto_bind` controls facade fallback binding and event delivery; OpenAMP’s native matching remains active. A callback can create additional endpoints in response to an announcement.
