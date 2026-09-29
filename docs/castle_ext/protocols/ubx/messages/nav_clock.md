# UBX NAV-CLOCK

## Overview

Defines the UBX NAV-CLOCK receiver-clock model containing bias, drift, and accuracy values.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_clock` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_clock::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_clock::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_CLOCK`). |
| `nav_clock::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`20U`). |
| `nav_clock::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_clock::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_clock` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `clk_b` | `int32_t` | `0` | Decoded `clk b` field. |
| `clk_d` | `int32_t` | `0` | Decoded `clk d` field. |
| `t_acc` | `uint32_t` | `0U` | Time accuracy estimate. |
| `f_acc` | `uint32_t` | `0U` | Associated accuracy/uncertainty value decoded from the payload. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_clock;

void handle(const message_view& raw) {
    nav_clock value{};
    if (nav_clock::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
