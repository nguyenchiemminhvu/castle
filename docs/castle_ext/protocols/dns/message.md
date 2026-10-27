# DNS Message Codec

## Overview
Encodes one recursive IN-class DNS question and decodes RFC 1035 responses. The decoder validates message bounds and transaction IDs, handles compression pointers in owner names and CNAME/NS/MX data, and skips unknown RR types using RDLENGTH.

## Header
`#include "castle_ext/protocols/dns/message.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle/container/string_view.hpp`](../../../../include/castle/container/string_view.hpp)
- [`castle_ext/protocols/dns/types.hpp`](types.md)

## Public API

| API | Description |
|---|---|
| `message::encode_query(name, type, id, output, capacity, written)` | Encodes a DNS header and exactly one QNAME/QTYPE/QCLASS question. Returns `full` for insufficient output capacity. |
| `message::is_truncated_packet(data, size)` | Checks the TC flag when at least four header bytes are available. |
| `message::parse_response(data, size, expected_id, response)` | Parses the header, question, and answer/authority/additional RRs into fixed-capacity storage. |
| `message::decode_name(data, size, offset, output, capacity, consumed)` | Decodes an uncompressed or compressed DNS name. `consumed` counts bytes at the original location only. |

All operations are O(packet size) in the worst case and allocation-free.

## Usage Example

```cpp
#include "castle_ext/protocols/dns/message.hpp"

uint8_t packet[512U]{};
castle::size_type packet_size = 0U;
auto status = castle::protocols::dns::message::encode_query(
    "example.com", castle::protocols::dns::record_type::a,
    0x1234U, packet, sizeof(packet), packet_size);
```

See [`samples/castle_ext/protocols/dns/message.cpp`](../../../../samples/castle_ext/protocols/dns/message.cpp).

## Constraints & Notes
The classic UDP message limit is 512 bytes; the codec itself accepts DNS messages up to 65535 bytes for TCP parsing. Domain names are ASCII presentation names with labels of 1–63 octets and a maximum expanded wire name of 255 octets. The response parser expects one question and rejects unsupported opcodes, malformed lengths, invalid compression pointers, and mismatched IDs. It retains A, AAAA, CNAME, NS, and MX records; unknown types and non-IN classes are safely skipped. DNSSEC, EDNS0, and multiple-question messages are outside the initial scope.
