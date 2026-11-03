# RPMsg Name Service

## Overview
Encodes and decodes the fixed-layout OpenAMP/Linux RPMsg Name Service payload and represents CREATE/DESTROY events.

## Header
`#include "castle_ext/io/rpmsg/name_service.hpp"`

## Dependencies
[`types.hpp`](../types.md), `castle/container/string.hpp`, `castle/error/status.hpp`.

## Public API
`name_service_event`, `decode_name_service_message()`, `encode_name_service_message()`, `name_service_message_size`.

## Usage Example
```cpp
#include "castle_ext/io/rpmsg/name_service.hpp"

int main()
{
    using namespace castle::io::rpmsg;
    name_service_event event;
    event.name.assign("sensor.service");
    event.endpoint_address = 2048U;
    uint8_t wire[name_service_message_size] = {};
    if (encode_name_service_message(event, wire, sizeof(wire)) != castle::status::ok) return 1;
    name_service_event decoded;
    return decode_name_service_message(wire, sizeof(wire), decoded) == castle::status::ok ? 0 : 2;
}
```

## Constraints & Notes
The payload is exactly 40 bytes: 32 name bytes, a 32-bit endpoint address, and 32-bit action flags (`CREATE=0`, `DESTROY=1`). The integer fields use the native endianness expected by the selected OpenAMP stack. Malformed lengths, unterminated/empty names, `ADDR_ANY`, reserved endpoint addresses, and unknown flags are rejected. The service name is limited to 31 visible ASCII/byte characters by the RPMsg field shape.
