# MQTT Packet Codec

## Overview
Provides checked MQTT fixed-header handling, canonical Remaining Length encoding/decoding, stream packet inspection, bounds-checked packet cursors, fixed-capacity MQTT 5 property serialization, and complete control-packet framing.

## Header
`#include "castle_ext/protocols/mqtt/codec.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/string_view.hpp`](../../../../include/castle/container/string_view.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle/utility/bytes.hpp`](../../../../include/castle/utility/bytes.hpp) (network-byte-order integer load/store)
- [`castle_ext/protocols/mqtt/types.hpp`](types.md)

## Public API
| Symbol | Description |
|---|---|
| `packet_view` | View of a complete packet in an input TCP stream. |
| `encode_remaining_length(value, out, written)` | Encodes MQTT Remaining Length in its minimal one-to-four-byte representation. |
| `decode_remaining_length(input, size, value, consumed)` | Decodes Remaining Length; returns `empty` for incomplete input and rejects malformed/noncanonical encodings. |
| `valid_fixed_header(type, flags)` | Checks packet-type-specific fixed flags, including PUBLISH QoS and DUP restrictions. |
| `inspect_packet(input, size, max_size, view)` | Inspects one complete frame without copying; returns `empty` for an incomplete stream frame. |
| `packet_writer` | Bounded cursor with `put_u8/u16/u32`, `put_bytes`, `put_utf8`, `put_binary`, and `put_varint`. |
| `packet_reader` | Bounds-checked cursor with matching read operations and `skip`. |
| `encode_properties` / `decode_properties` | Encode/decode a bounded MQTT 5 property section. Unknown IDs can optionally be skipped. |
| `encode_packet(type, flags, body, size, output, capacity, written)` | Builds the fixed header and Remaining Length around a caller-owned packet body. |

## Usage Example
```cpp
#include "castle_ext/protocols/mqtt/codec.hpp"

int main()
{
    uint8_t frame[16U]{};
    castle::size_type written = 0U;
    if (castle::protocols::mqtt::encode_packet(
            castle::protocols::mqtt::packet_type::pingreq, 0U,
            nullptr, 0U, frame, sizeof(frame), written) != castle::status::ok) { return 1; }
    castle::protocols::mqtt::packet_view packet;
    return castle::protocols::mqtt::inspect_packet(frame, written, sizeof(frame), packet) == castle::status::ok ? 0 : 2;
}
```

## Constraints & Notes
The codec never allocates. The caller owns input/output buffers and chooses maximum packet size. Remaining Length is limited to 268,435,455 bytes by MQTT, while practical users should configure a much smaller packet maximum. `inspect_packet` never consumes bytes; callers advance by `packet_size` after handling. Property list entries are bounded by compile-time capacities and only known property wire types are encoded.
