# UBX MON-MSGPP

## Overview

Model of UBX-MON-MSGPP (0x0a 0x06), the 120-byte message parse and process status (periodic/polled, deprecated in favour of UBX-MON-COMMS). The spec fields `msg1`..`msg6` (one `U2[8]` per port 0..5) are stored as one 6x8 table so ports can be iterated: `msg[p]` is spec `msg<p+1>`, and `msg[p][k]` is the count of successfully parsed messages of protocol `k` on port `p`. Built on `detail::fixed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_msgpp.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/array.hpp](../../../../container/array.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [detail/fixed_message.hpp](detail/fixed_message.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_msgpp` | `detail::fixed_message<mon_msgpp, UBX_CLASS_MON, UBX_ID_MON_MSGPP, 120U>` |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x06` | UBX class / ID. |
| `payload_length` | `120` | Exact payload length. |
| `num_ports` | `6` | Rows of `msg` / elements of `skipped`. |
| `num_protocols` | `8` | Columns of `msg`. |

### Fields

| Field | Type | Offset | Unit | Description |
| --- | --- | --- | --- | --- |
| `msg` | `array<array<uint16_t, 8>, 6>` | 0 + p·16 | msgs | `msg[p]` = spec `msg<p+1>`: parsed-message count per protocol on port `p`. |
| `skipped` | `array<uint32_t, 6>` | 96 | bytes | Skipped bytes per port. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | `true` for class `0x0A` / ID `0x06`. |
| `static bool decode(const message_view&, mon_msgpp&)` | Decodes a `120`-byte payload; `false` for wrong class/ID or any other length. |
| `void encode(payload_writer&) const` | Writes the payload (reserved bytes as zero); used by `encode_message()`. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using castle::protocols::ubx::messages::mon_msgpp;

void on_message(const message_view& raw) {
    mon_msgpp status{};
    if (mon_msgpp::decode(raw, status)) {
        uint16_t port1_protocol0 = status.msg[1U][0U]; // spec msg2[0]
        uint32_t port1_skipped = status.skipped[1U];
        (void)port1_protocol0; (void)port1_skipped;
    }
}
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_msgpp.cpp`.

## Constraints & Notes

- Payload length must be exactly 120 bytes.
- The spec does not name the protocol index order inside each `U2[8]`; it is exposed as-is.
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
