# UBX NAV-TIMEUTC

## Overview

Defines the UBX NAV-TIMEUTC UTC-time solution model.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_timeutc` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_timeutc::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_timeutc::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_TIMEUTC`). |
| `nav_timeutc::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`20U`). |
| `nav_timeutc::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_timeutc::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_timeutc` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `t_acc` | `uint32_t` | `0U` | Time accuracy estimate. |
| `nano` | `int32_t` | `0` | Nanosecond/fractional-time component. |
| `year` | `uint16_t` | `0U` | Calendar year. |
| `month` | `uint8_t` | `0U` | Calendar month. |
| `day` | `uint8_t` | `0U` | Calendar day. |
| `hour` | `uint8_t` | `0U` | Decoded `hour` field. |
| `min` | `uint8_t` | `0U` | Minute component. |
| `sec` | `uint8_t` | `0U` | Second component. |
| `valid` | `uint8_t` | `0U` | True when decoding completed successfully for the sentence. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timeutc;

void handle(const message_view& raw) {
    nav_timeutc value{};
    if (nav_timeutc::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
