# UBX NAV2-EELL

## Overview

Defines the UBX NAV2-EELL estimated-position-error ellipse data model.

## Header

`#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"`

## Dependencies

- [`castle_ext/protocols/ubx/messages/nav_eell.hpp`](nav_eell.md)

## Public API

| API | Description |
| --- | --- |
| `nav2_eell` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav2_eell::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV2`). |
| `nav2_eell::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV2_EELL`). |
| `nav2_eell::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav2_eell::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav2_eell;

void handle(const message_view& raw) {
    nav2_eell value{};
    if (nav2_eell::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
