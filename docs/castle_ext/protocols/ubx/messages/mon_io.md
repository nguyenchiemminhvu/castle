# UBX MON-IO

## Overview

Defines the UBX MON-IO decoded model with one fixed-capacity record per monitored I/O port.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_io.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `mon_io_port` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `mon_io<MaxPorts=8U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `mon_io::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_MON`). |
| `mon_io::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_MON_IO`). |
| `mon_io::block_length` | `static constexpr castle::size_type` protocol/configuration constant (`20U`). |
| `mon_io::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `mon_io::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `mon_io_port` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `rx_bytes` | `uint32_t` | `0U` | Received byte count for the port. |
| `tx_bytes` | `uint32_t` | `0U` | Transmitted byte count for the port. |
| `parity_errs` | `uint16_t` | `0U` | Parity error counter. |
| `framing_errs` | `uint16_t` | `0U` | Framing error counter. |
| `overrun_errs` | `uint16_t` | `0U` | Overrun error counter. |
| `break_cond` | `uint16_t` | `0U` | Break-condition counter/status field. |
| `rx_busy` | `uint8_t` | `0U` | Receive-busy status. |
| `tx_busy` | `uint8_t` | `0U` | Transmit-busy status. |

### `mon_io` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `num_ports` | `uint8_t` | `0U` | Number of I/O port records decoded. |
| `ports` | `castle::container::array<mon_io_port,MaxPorts>` | `—` | Fixed-capacity I/O port records. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_io.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::mon_io;

void handle(const message_view& raw) {
    mon_io<4U> value{};
    if (mon_io::decode(raw, value)) {
        // The fixed-capacity arrays contain at most the configured bound.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload must be non-empty and its size must be an exact multiple of the record block length.
- Template capacity parameters bound retained records/text; excess protocol records are skipped or ignored after validation rather than dynamically allocated.
