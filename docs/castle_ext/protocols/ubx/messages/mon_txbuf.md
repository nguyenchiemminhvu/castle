# UBX MON-TXBUF

## Overview

Defines the fixed-size UBX MON-TXBUF transmit-buffer usage model for the protocol-defined target set.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_txbuf.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `mon_txbuf` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `mon_txbuf::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_MON`). |
| `mon_txbuf::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_MON_TXBUF`). |
| `mon_txbuf::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`28U`). |
| `mon_txbuf::num_targets` | `static constexpr castle::size_type` protocol/configuration constant (`6U`). |
| `mon_txbuf::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `mon_txbuf::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `mon_txbuf` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `pending` | `castle::container::array<uint16_t,num_targets>` | `—` | Pending transmit-buffer values for the protocol-defined target set. |
| `usage` | `castle::container::array<uint8_t,num_targets>` | `—` | Current transmit-buffer usage values. |
| `peak_usage` | `castle::container::array<uint8_t,num_targets>` | `—` | Peak transmit-buffer usage values. |
| `t_used` | `uint8_t` | `0U` | Total used transmit-buffer metric. |
| `t_peak` | `uint8_t` | `0U` | Total peak transmit-buffer metric. |
| `errors` | `uint8_t` | `0U` | Transmit-buffer error count/status. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_txbuf.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::mon_txbuf;

void handle(const message_view& raw) {
    mon_txbuf value{};
    if (mon_txbuf::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
