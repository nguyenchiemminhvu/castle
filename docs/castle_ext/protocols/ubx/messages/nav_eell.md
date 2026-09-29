# UBX NAV-EELL

## Overview

Defines the UBX NAV-EELL position-error ellipse model.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_eell` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_eell::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_eell::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_EELL`). |
| `nav_eell::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`16U`). |
| `nav_eell::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_eell::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_eell` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `err_maj` | `uint16_t` | `0U` | Major-axis position-error estimate. |
| `err_min` | `uint16_t` | `0U` | Minor-axis position-error estimate. |
| `err_orient` | `uint16_t` | `0U` | Position-error ellipse orientation. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_eell;

void handle(const message_view& raw) {
    nav_eell value{};
    if (nav_eell::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
