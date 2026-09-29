# UBX ACK

## Overview

Defines typed UBX ACK message models plus a compatibility aggregate that represents both ACK-ACK and ACK-NAK.

## Header

`#include "castle_ext/protocols/ubx/messages/ack.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `ack_message<MessageId>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `ack_message::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_ACK`). |
| `ack_message::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`MessageId`). |
| `ack_message::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`2U`). |
| `ack_message::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `ack_message::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |
| `ack` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `ack::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`2U`). |
| `ack::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `ack::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |
| `ack_ack` | Alias for `ack_message<UBX_ID_ACK_ACK>`. |
| `ack_nak` | Alias for `ack_message<UBX_ID_ACK_NAK>`. |

### `ack_message` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `cls_id` | `uint8_t` | `0U` | Class byte carried by ACK-ACK/ACK-NAK payload. |
| `msg_id_` | `uint8_t` | `0U` | Message ID byte carried by the ACK payload. |

### `ack` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `acked_class` | `uint8_t` | `0U` | Class of the UBX message being acknowledged. |
| `acked_id` | `uint8_t` | `0U` | ID of the UBX message being acknowledged. |
| `accepted` | `bool` | `false` | True for ACK-ACK and false for ACK-NAK. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/ack.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::ack;

void handle(const message_view& raw) {
    ack value{};
    if (ack::decode(raw, value)) {
        // `value.accepted` distinguishes ACK-ACK from ACK-NAK.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
- `ack_message<MessageId>` gives a strongly typed ACK-ACK/ACK-NAK specialization; `ack` is the compatibility aggregate that accepts both IDs.
