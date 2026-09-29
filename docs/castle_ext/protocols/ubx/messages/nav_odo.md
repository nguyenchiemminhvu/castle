# UBX NAV-ODO

## Overview

Defines the UBX NAV-ODO odometer model with instantaneous, total, and standard-deviation distance values.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_odo.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_odo` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_odo::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_odo::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_ODO`). |
| `nav_odo::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`20U`). |
| `nav_odo::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_odo::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_odo` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `distance` | `uint32_t` | `0U` | Decoded `distance` field. |
| `total_distance` | `uint32_t` | `0U` | Decoded `total distance` field. |
| `distance_std` | `uint32_t` | `0U` | Decoded `distance std` field. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_odo.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_odo;

void handle(const message_view& raw) {
    nav_odo value{};
    if (nav_odo::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
