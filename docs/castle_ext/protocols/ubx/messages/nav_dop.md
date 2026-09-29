# UBX NAV-DOP

## Overview

Defines the UBX NAV-DOP dilution-of-precision model with the seven protocol DOP terms.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_dop` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_dop::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_dop::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_DOP`). |
| `nav_dop::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`18U`). |
| `nav_dop::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_dop::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_dop` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `g_dop` | `uint16_t` | `0U` | Geometric dilution of precision. |
| `p_dop` | `uint16_t` | `0U` | Position dilution of precision. |
| `t_dop` | `uint16_t` | `0U` | Time dilution of precision. |
| `v_dop` | `uint16_t` | `0U` | Vertical dilution of precision. |
| `h_dop` | `uint16_t` | `0U` | Horizontal dilution of precision. |
| `n_dop` | `uint16_t` | `0U` | Northing dilution of precision. |
| `e_dop` | `uint16_t` | `0U` | Easting dilution of precision. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_dop;

void handle(const message_view& raw) {
    nav_dop value{};
    if (nav_dop::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
