# UBX MON-IO

## Overview

Model of UBX-MON-IO (0x0a 0x02), the per-port I/O system status (periodic/polled, deprecated in favour of UBX-MON-COMMS). The payload is a sequence of 20-byte port blocks with no header and no count field (`[0..n]·20`); the port count is implied by the payload length, and the decoded blocks are stored in a fixed-capacity vector.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_io.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_io_port` | One 20-byte port block (`wire_size = 20`, `layout()`). |
| `mon_io<MaxPorts = 8>` | Message model; stores up to `MaxPorts` port blocks. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `mon_io::msg_class` / `msg_id` | `0x0A` / `0x02` | UBX class / ID. |
| `mon_io::header_length` | `0` | No header before the repeated group. |
| `mon_io::block_length` | `20` | Port block size. |

### `mon_io_port` fields

| Field | Type | Offset | Unit | Description |
| --- | --- | --- | --- | --- |
| `rx_bytes` | `uint32_t` | 0 + n·20 | bytes | Number of bytes ever received. |
| `tx_bytes` | `uint32_t` | 4 + n·20 | bytes | Number of bytes ever sent. |
| `parity_errs` | `uint16_t` | 8 + n·20 | - | 100 ms timeslots with parity errors. |
| `framing_errs` | `uint16_t` | 10 + n·20 | - | 100 ms timeslots with framing errors. |
| `overrun_errs` | `uint16_t` | 12 + n·20 | - | 100 ms timeslots with overrun errors. |
| `break_cond` | `uint16_t` | 14 + n·20 | - | 100 ms timeslots with break conditions. |
| (reserved0) | `U1[4]` | 16 + n·20 | - | Skipped on decode, written as zero. |

### `mon_io` fields

| Field | Type | Description |
| --- | --- | --- |
| `ports` | `castle::container::vector<mon_io_port, MaxPorts>` | Port blocks in wire order; `ports.size()` is the number stored. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | `true` for class `0x0A` / ID `0x02`. |
| `static bool decode(const message_view&, mon_io&)` | `false` for wrong class/ID or a length that is not a multiple of 20; an empty payload decodes to zero ports. |
| `void encode(payload_writer&) const` | Writes every stored port block. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using castle::protocols::ubx::messages::mon_io;

void on_message(const message_view& raw) {
    mon_io<6U> io{};
    if (mon_io<6U>::decode(raw, io)) {
        for (const auto& port : io.ports) {
            (void)port.rx_bytes;
        }
    }
}
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_io.cpp`.

## Constraints & Notes

- Payload length must be a whole multiple of 20 bytes (`implied_group_count`). Zero ports (an empty payload, which `decode_frame()` presents as a null view) is accepted explicitly.
- Ports beyond `MaxPorts` are skipped; `ports.size()` is then `MaxPorts`. The on-wire count is `raw.payload_length() / 20`.
- Earlier versions exposed `rx_busy` / `tx_busy` and `num_ports`. `rx_busy` / `tx_busy` were not in the spec (bytes 16..19 are `reserved0`) and are removed; use `ports.size()` instead of `num_ports`.
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
