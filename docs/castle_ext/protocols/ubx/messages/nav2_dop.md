# UBX NAV2-DOP

## Overview

Defines the UBX NAV2-DOP dilution-of-precision data model.

## Header

`#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"`

## Dependencies

- [`castle_ext/protocols/ubx/messages/nav_dop.hpp`](nav_dop.md)

## Public API

| API | Description |
| --- | --- |
| `nav2_dop` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav2_dop::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV2`). |
| `nav2_dop::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV2_DOP`). |
| `nav2_dop::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav2_dop::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::nav2_dop;

void handle(const message_view& raw) {
    nav2_dop value{};
    if (nav2_dop::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
