# UBX MON-RXBUF

## Overview

Model of UBX-MON-RXBUF (0x0a 0x07), the 24-byte receiver buffer status (periodic/polled, deprecated in favour of UBX-MON-COMMS). Per-target arrays are indexed by I/O target (port) number 0..5. Built on `detail::fixed_message`, so decode and encode share one layout.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_rxbuf.hpp"`

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
| `mon_rxbuf` | `detail::fixed_message<mon_rxbuf, UBX_CLASS_MON, UBX_ID_MON_RXBUF, 24U>` |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x07` | UBX class / ID. |
| `payload_length` | `24` | Exact payload length. |
| `num_targets` | `6` | Number of I/O targets. |

### Fields

| Field | Type | Offset | Unit | Description |
| --- | --- | --- | --- | --- |
| `pending` | `array<uint16_t, 6>` | 0 | bytes | Bytes pending in the receiver buffer per target. |
| `usage` | `array<uint8_t, 6>` | 12 | % | Maximum usage during the last sysmon period per target. |
| `peak_usage` | `array<uint8_t, 6>` | 18 | % | Maximum usage per target. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | `true` for class `0x0A` / ID `0x07`. |
| `static bool decode(const message_view&, mon_rxbuf&)` | Decodes a `24`-byte payload; `false` for wrong class/ID or any other length. |
| `void encode(payload_writer&) const` | Writes the payload (reserved bytes as zero); used by `encode_message()`. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using castle::protocols::ubx::messages::mon_rxbuf;

void on_message(const message_view& raw) {
    mon_rxbuf status{};
    if (mon_rxbuf::decode(raw, status)) {
        uint8_t uart1_peak = status.peak_usage[1U];
        (void)uart1_peak;
    }
}

// Poll: encode_poll<mon_rxbuf>(frame, sizeof(frame), written);
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_rxbuf.cpp`.

## Constraints & Notes

- Payload length must be exactly 24 bytes.
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
