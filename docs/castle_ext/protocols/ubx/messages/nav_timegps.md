# UBX NAV-TIMEGPS

## Overview

Defines the UBX NAV-TIMEGPS GPS-time solution model.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timegps.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_timegps` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_timegps::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_timegps::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_TIMEGPS`). |
| `nav_timegps::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`16U`). |
| `nav_timegps::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_timegps::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_timegps` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `f_tow` | `int32_t` | `0` | Fractional time-of-week value. |
| `week` | `int16_t` | `0` | GPS week number. |
| `leap_s` | `int8_t` | `0` | Leap-second field. |
| `valid` | `uint8_t` | `0U` | True when decoding completed successfully for the sentence. |
| `t_acc` | `uint32_t` | `0U` | Time accuracy estimate. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timegps.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::nav_timegps;

void handle(const message_view& raw) {
    nav_timegps value{};
    if (nav_timegps::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
