# UBX NAV-ATT

## Overview

Defines the UBX NAV-ATT attitude model with time, message version, roll/pitch/heading, and corresponding accuracy fields.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_att.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_att` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_att::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_att::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_ATT`). |
| `nav_att::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`32U`). |
| `nav_att::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_att::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_att` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `roll` | `int32_t` | `0` | Decoded `roll` field. |
| `pitch` | `int32_t` | `0` | Decoded `pitch` field. |
| `heading` | `int32_t` | `0` | Decoded `heading` field. |
| `acc_roll` | `uint32_t` | `0U` | Decoded `acc roll` field. |
| `acc_pitch` | `uint32_t` | `0U` | Decoded `acc pitch` field. |
| `acc_heading` | `uint32_t` | `0U` | Decoded `acc heading` field. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_att.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::nav_att;

void handle(const message_view& raw) {
    nav_att value{};
    if (nav_att::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
