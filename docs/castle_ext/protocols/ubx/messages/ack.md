# UBX ACK-ACK / ACK-NAK

## Overview

Typed models of UBX-ACK-ACK (class 0x05, ID 0x01) and UBX-ACK-NAK (class 0x05, ID 0x00). The receiver sends one of them, within one second, after processing an input message such as UBX-CFG-VALSET, naming the class and ID of the message it accepted or rejected. Both IDs carry the same 2-byte payload, so one template models both. The `ack` aggregate accepts either ID and reports which one arrived.

## Header

`#include "castle_ext/protocols/ubx/messages/ack.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `ack_message<MsgId>` | Payload model; derives from `detail::fixed_message<ack_message<MsgId>, UBX_CLASS_ACK, MsgId, 2U>`. |
| `ack_ack` | `ack_message<UBX_ID_ACK_ACK>`: message acknowledged. |
| `ack_nak` | `ack_message<UBX_ID_ACK_NAK>`: message not acknowledged. |
| `ack` | Aggregate matching both IDs. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `ack_message::msg_class` | `uint8_t` | `UBX_CLASS_ACK` (0x05) | UBX class. |
| `ack_message::msg_id` | `uint8_t` | `MsgId` (ACK 0x01, NAK 0x00) | UBX message ID. |
| `ack_message::payload_length` | `castle::size_type` | `2U` | Exact payload length. |
| `ack::msg_class` | `uint8_t` | `UBX_CLASS_ACK` | UBX class. |
| `ack::payload_length` | `castle::size_type` | `2U` | Exact payload length. |

### Fields

| Offset | `ack_message` field | `ack` field | Type | Description |
| --- | --- | --- | --- | --- |
| 0 | `cls_id` | `acked_class` | `uint8_t` | Class of the acknowledged or rejected message. |
| 1 | `acked_msg_id` | `acked_id` | `uint8_t` | Message ID of the acknowledged or rejected message. |
| - | - | `accepted` | `bool` | `true` for ACK-ACK, `false` for ACK-NAK. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool ack_message::matches(const message_view& raw)` | `bool` | True for class 0x05 / `MsgId` only. |
| `static bool ack_message::decode(const message_view& raw, ack_message& out)` | `bool` | Decodes an exactly 2-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void ack_message::encode(payload_writer& writer) const` | - | Writes the payload. Used for loop-back and simulation through `encode_message()`. |
| `static bool ack::matches(const message_view& raw)` | `bool` | True for ACK-ACK or ACK-NAK. |
| `static bool ack::decode(const message_view& raw, ack& out)` | `bool` | Decodes an exactly 2-byte payload of either ID and sets `accepted`. On failure returns `false` and leaves `out` value-initialised. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/ack.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

bool valset_accepted(const message_view& raw)
{
    ack reply{};
    return ack::decode(raw, reply)
        && (reply.acked_class == UBX_CLASS_CFG)
        && (reply.acked_id == UBX_ID_CFG_VALSET)
        && reply.accepted;
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/ack.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/ack.cpp).

## Constraints & Notes

- Subscribe to `ack_ack` and `ack_nak` separately in a decoder registry, or to `ack` to get both through one handler.
- `decode()` accepts only a payload of exactly 2 bytes.
- Fixed-size value types: no heap, exceptions, RTTI or virtual functions.
- `message_view` is non-owning. Its payload must stay valid while `decode()` runs.
- Thread-safety: none. Instances are plain values and are not thread-safe.
