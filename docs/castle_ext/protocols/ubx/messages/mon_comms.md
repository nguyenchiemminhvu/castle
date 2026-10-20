# UBX MON-COMMS

## Overview

Model of UBX-MON-COMMS (class `0x0A`, ID `0x36`), the consolidated communication-port report: an 8-byte header (TX error flags, four protocol IDs) followed by one 40-byte block per active port (`8 + nPorts·40`). Each port's `msgs[i]` counts messages parsed for the protocol named by the header's `prot_ids[i]`. Supersedes the deprecated UBX-MON-IO / MON-TXBUF / MON-RXBUF / MON-MSGPP.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_comms.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)
- [castle/container/array.hpp](../../../../container/array.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_comms_protocol` | `enum class : uint8_t` for `protIds`: `ubx = 0`, `nmea = 1`, `rtcm2 = 2`, `rtcm3 = 5`, `spartn = 6`, `none = 0xFF`. |
| `mon_comms_port` | One 40-byte port block. |
| `mon_comms<MaxPorts = 5U>` | Message with up to `MaxPorts` stored ports. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `MON_COMMS_PROTOCOL_SLOTS` | `4U` | Entries of `prot_ids` and `msgs`. |
| `mon_comms::msg_class` / `msg_id` | `0x0A` / `0x36` | Class/ID. |
| `mon_comms::header_length` | `8U` | Header bytes. |
| `mon_comms::block_length` | `40U` | Bytes per port block. |
| `mon_comms::TX_ERRORS_MEM` | `0x01U` | `txErrors` bit 0: memory allocation error. |
| `mon_comms::TX_ERRORS_ALLOC` | `0x02U` | `txErrors` bit 1: allocation error (TX buffer full). |

### `mon_comms` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | 0 | Message version (`0x00`). |
| `n_ports` | `uint8_t` | 1 | On-wire port count; may exceed `ports.size()`. |
| `tx_errors` | `uint8_t` (X1) | 2 | See `TX_ERRORS_*`. |
| — | reserved | 3 | |
| `prot_ids` | `castle::container::array<mon_comms_protocol, 4>` | 4 | Protocol of each `msgs` slot (defaults to `none`). |
| `ports` | `castle::container::vector<mon_comms_port, MaxPorts>` | 8 + n·40 | Stored port blocks. |

### `mon_comms_port` fields (offset within block)

| Field | Type | Offset | Unit | Description |
| --- | --- | --- | --- | --- |
| `port_id` | `uint16_t` | 0 | - | Unique port identifier (Integration manual). |
| `tx_pending` | `uint16_t` | 2 | bytes | Pending in transmitter buffer. |
| `tx_bytes` | `uint32_t` | 4 | bytes | Ever sent. |
| `tx_usage` | `uint8_t` | 8 | % | Max TX buffer usage, last sysmon period. |
| `tx_peak_usage` | `uint8_t` | 9 | % | Max TX buffer usage. |
| `rx_pending` | `uint16_t` | 10 | bytes | In receiver buffer. |
| `rx_bytes` | `uint32_t` | 12 | bytes | Ever received. |
| `rx_usage` | `uint8_t` | 16 | % | Max RX buffer usage, last sysmon period. |
| `rx_peak_usage` | `uint8_t` | 17 | % | Max RX buffer usage. |
| `overrun_errs` | `uint16_t` | 18 | - | 100 ms timeslots with overrun errors. |
| `msgs` | `castle::container::array<uint16_t, 4>` | 20 | msg | Parsed messages per `prot_ids` slot. |
| — | reserved | 28..35 | | |
| `skipped` | `uint32_t` | 36 | bytes | Skipped bytes. |

### Functions

| Function | Description |
| --- | --- |
| `static bool mon_comms::matches(const message_view&)` | Class/ID check. |
| `static bool mon_comms::decode(const message_view&, mon_comms&)` | Validates `8 + nPorts·40` exactly, then decodes. |
| `void mon_comms::encode(payload_writer&) const` | Writes header (`nPorts = ports.size()`) and every port. |
| `bool tx_error_mem() const` / `bool tx_error_alloc() const` | `txErrors` bits. |
| `bool parsed_messages(const mon_comms_port&, mon_comms_protocol, uint16_t& count) const` | Finds the first slot reporting the protocol and returns its `msgs` count; `false` (count 0) when no slot reports it. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_comms.hpp"

using namespace castle::protocols::ubx::messages;

mon_comms<> comms{};
if (mon_comms<>::decode(raw, comms))
{
    for (const mon_comms_port& port : comms.ports)
    {
        uint16_t ubx_count = 0U;
        if (comms.parsed_messages(port, mon_comms_protocol::ubx, ubx_count)) { /* ... */ }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/mon_comms.cpp`.

## Constraints & Notes

- Storage: `castle::container::vector<..., MaxPorts>` (default `MaxPorts = 5U`) lives inside the object; no heap allocation, no exceptions.
- `decode()` requires the payload length to be exactly `8 + nPorts·40`; any mismatch (or a different class/ID, or fewer than 8 bytes) returns `false` with `out` value-initialised.
- Capacity overflow: when `nPorts` exceeds `MaxPorts`, the first `MaxPorts` blocks are stored, the rest are skipped, `decode()` still succeeds and `n_ports` reports the on-wire count.
- `encode()` writes the count from the vector's `size()`; the stored count field is ignored.
- Reserved bytes are skipped on decode and written as zero on encode.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none; not thread-safe. Value type; copy freely.
