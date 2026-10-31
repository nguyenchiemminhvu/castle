# CoAP Codec

## Overview
Encodes and decodes complete CoAP datagrams using the RFC 7252 four-byte base header, token, delta-encoded options, payload marker, and bounded payload storage.

## Header
`#include "castle_ext/protocols/coap/codec.hpp"`

## Dependencies
- [Core compiler macros](../../../../include/castle/core/compiler.hpp)
- [Core types](../../../../include/castle/core/types.hpp)
- [Status](../../../../include/castle/error/status.hpp)
- [Byte helpers](../../../../include/castle/utility/bytes.hpp) (network-byte-order integer load/store)
- [CoAP Types](types.md)

## Public API

| API | Description |
|---|---|
| `encode_message(message, output, capacity, written)` | Encodes a complete message; returns `full` when the output buffer is too small. |
| `decode_message(input, size, message, decode_error*)` | Parses one datagram; skips unknown elective options and reports unknown critical options. |
| `decode_uint_option(value, length, output)` | Decodes an unsigned integer option (zero to four bytes). |
| `valid_option_length`, `is_repeatable_option` | Basic RFC option length and repetition checks for implemented options. |
| `decode_error` | Distinguishes malformed headers/options, unsupported version, unknown critical option, and capacity exhaustion. |

## Usage Example

```cpp
#include "castle_ext/protocols/coap/codec.hpp"

int main()
{
    using namespace castle::protocols::coap;
    message<> request;
    request.code_value = static_cast<uint8_t>(code::get);
    request.message_id = 42U;
    if (append_uri_path(request, "well-known/core") != castle::status::ok) return 1;
    uint8_t packet[256U]{};
    castle::size_type packet_size = 0U;
    if (encode_message(request, packet, sizeof(packet), packet_size) != castle::status::ok) return 2;
    message<> parsed;
    return decode_message(packet, packet_size, parsed) == castle::status::ok ? 0 : 3;
}
```

## Constraints & Notes
The codec performs no allocation and never reads beyond the supplied datagram. The caller chooses maximum options, per-option value size, and payload size through the message type. Unknown elective options are ignored; unknown critical options (or known options with invalid lengths) are reported so an endpoint can reject them. This is a message codec, not a URI parser, DTLS layer, or Block-Wise transfer manager.
