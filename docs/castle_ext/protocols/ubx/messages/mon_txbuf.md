# UBX MON-TXBUF

## Overview

Model of UBX-MON-TXBUF (0x0a 0x08), the 28-byte transmitter buffer status (periodic/polled, deprecated in favour of UBX-MON-COMMS). Per-target arrays are indexed by I/O target (port) number 0..5. The `errors` bitmask is exposed raw plus `ERRORS_*` masks and accessors. Built on `detail::fixed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_txbuf.hpp"`

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
| `mon_txbuf` | `detail::fixed_message<mon_txbuf, UBX_CLASS_MON, UBX_ID_MON_TXBUF, 28U>` |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x08` | UBX class / ID. |
| `payload_length` | `28` | Exact payload length. |
| `num_targets` | `6` | Number of I/O targets. |
| `ERRORS_LIMIT` | `0x3F` | `errors` bits 5..0 `limit`: bit N = buffer limit of target N reached. |
| `ERRORS_MEM` | `0x40` | `errors` bit 6 `mem`: memory allocation error. |
| `ERRORS_ALLOC` | `0x80` | `errors` bit 7 `alloc`: allocation error (TX buffer full). |

### Fields

| Field | Type | Offset | Unit | Description |
| --- | --- | --- | --- | --- |
| `pending` | `array<uint16_t, 6>` | 0 | bytes | Bytes pending in the transmitter buffer per target. |
| `usage` | `array<uint8_t, 6>` | 12 | % | Maximum usage during the last sysmon period per target. |
| `peak_usage` | `array<uint8_t, 6>` | 18 | % | Maximum usage per target. |
| `t_used` | `uint8_t` | 24 | % | Spec `tUsage`: maximum usage during the last sysmon period, all targets. |
| `t_peak` | `uint8_t` | 25 | % | Spec `tPeakusage`: maximum usage, all targets. |
| `errors` | `uint8_t` | 26 | - | Error bitmask (`ERRORS_*`). |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | `true` for class `0x0A` / ID `0x08`. |
| `static bool decode(const message_view&, mon_txbuf&)` | Decodes a `28`-byte payload; `false` for wrong class/ID or any other length. |
| `void encode(payload_writer&) const` | Writes the payload (reserved bytes as zero); used by `encode_message()`. |
| `uint8_t limit() const` | `errors & ERRORS_LIMIT` (6-bit per-target limit mask). |
| `bool limit_reached(size_type target) const` | Bit `target` of `limit()`; `false` for `target >= 6`. |
| `bool mem_error() const` | `ERRORS_MEM` set. |
| `bool alloc_error() const` | `ERRORS_ALLOC` set. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using castle::protocols::ubx::messages::mon_txbuf;

void on_message(const message_view& raw) {
    mon_txbuf status{};
    if (mon_txbuf::decode(raw, status) && (status.alloc_error() || status.limit_reached(1U))) {
        // UART1 output is being dropped: lower the output rate.
    }
}
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_txbuf.cpp`.

## Constraints & Notes

- Payload length must be exactly 28 bytes.
- Field names `t_used` / `t_peak` are kept from the previous API; they map to the spec names `tUsage` / `tPeakusage`.
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
