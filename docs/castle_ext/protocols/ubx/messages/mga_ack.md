# UBX MGA-ACK

## Overview

Models UBX-MGA-ACK (0x13 0x60), the 8-byte output the receiver sends to acknowledge an assistance message when CFG-NAVSPG-ACKAIDING is enabled. It has one layout, so it is a plain `detail::fixed_message`. The ACK that ends a UBX-MGA-DBD poll carries the number of entries sent in `msgPayloadStart`.

## Header

`#include "castle_ext/protocols/ubx/messages/mga_ack.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle/utility/bytes.hpp`](../../../../utility/bytes.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

| Type / constant | Values | Description |
| --- | --- | --- |
| `mga_ack` | `fixed_message<mga_ack, UBX_CLASS_MGA, UBX_ID_MGA_ACK, 8U>` | Output payload model (spec 3.13.1.1). |
| `enum class mga_ack_type : uint8_t` | `not_used`=0, `accepted`=1 | Whether the acknowledged message was used. |
| `enum class mga_ack_info_code : uint8_t` | `accepted`=0, `no_time`=1, `version_not_supported`=2, `size_mismatch`=3, `store_failed`=4, `not_ready`=5, `unknown_type`=6 | Why the receiver did (not) use the message. |
| `MGA_ACK_VERSION` | 0x00 | Message version byte (checked by `decode()`). |
| `mga_ack::msg_class` / `msg_id` / `payload_length` | `UBX_CLASS_MGA`, `UBX_ID_MGA_ACK`, `8U` | Wire constants. |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True for class 0x13, ID 0x60. |
| `static bool decode(const message_view&, mga_ack&)` | Decodes the 8-byte payload; `false` (and `out` reset) on wrong class/ID, wrong length or a non-zero version byte. |
| `void encode(payload_writer&) const` | Writes the payload (loop-back / tests). |

### `mga_ack` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `type` | `mga_ack_type` | `mga_ack_type::not_used` | Acknowledgement type (spec `type`). |
| `info_code` | `mga_ack_info_code` | `mga_ack_info_code::accepted` | Receiver outcome (spec `infoCode`). |
| `acked_msg_id` | `uint8_t` | `0U` | UBX ID of the acknowledged MGA message (spec `msgId`; renamed to avoid `msg_id`). |
| `msg_payload_start` | `castle::container::array<uint8_t, 4U>` | `{}` | First 4 payload bytes of the acknowledged message. |

### `mga_ack` helpers

| Function | Returns | Description |
| --- | --- | --- |
| `accepted()` | `bool` | Reports `type == accepted`. |
| `dbd_entry_count()` | `uint32_t` | `msg_payload_start` read as a little-endian U4 (entries sent after a UBX-MGA-DBD poll). |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_ack.hpp"

using castle::protocols::ubx::message_view;
using namespace castle::protocols::ubx::messages;

void on_frame(const message_view& raw) {
    mga_ack ack{};
    if (mga_ack::decode(raw, ack) && !ack.accepted()) {
        if (ack.info_code == mga_ack_info_code::no_time) {
            // send UBX-MGA-INI-TIME_UTC first, then retry ack.acked_msg_id
        }
    }
}
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_ack.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Payload length must be exactly 8 bytes.
- Spec `msgId` is exposed as `acked_msg_id` because `msg_id` is the model's own UBX ID constant (same convention as `ack::acked_id`).
- `type` and `info_code` are stored as the enums; values outside the spec range are kept as-is (cast through the underlying `uint8_t`).
