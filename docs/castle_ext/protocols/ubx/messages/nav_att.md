# UBX NAV-ATT

## Overview

Typed model of UBX-NAV-ATT (class 0x01, ID 0x05), the periodic/polled vehicle attitude solution: roll, pitch and heading with their accuracy estimates. A zero accuracy means the corresponding angle is not available.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_att.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md) (`castle::math::pow10`)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_att` | Value type for the payload; derives from `detail::fixed_message<nav_att, UBX_CLASS_NAV, UBX_ID_NAV_ATT, 32U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_att::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_att::msg_id` | `uint8_t` | `UBX_ID_NAV_ATT` (0x05) | UBX message ID. |
| `nav_att::payload_length` | `castle::size_type` | `32U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `version` | `uint8_t` | - | Message version (0x00 for this version). |
| 5 | - | `U1[3]` | - | Reserved; skipped on decode, zero on encode. |
| 8 | `roll` | `int32_t` | 1e-5 deg | Vehicle roll. |
| 12 | `pitch` | `int32_t` | 1e-5 deg | Vehicle pitch. |
| 16 | `heading` | `int32_t` | 1e-5 deg | Vehicle heading. |
| 20 | `acc_roll` | `uint32_t` | 1e-5 deg | Roll accuracy (0 = roll not available). |
| 24 | `acc_pitch` | `uint32_t` | 1e-5 deg | Pitch accuracy (0 = pitch not available). |
| 28 | `acc_heading` | `uint32_t` | 1e-5 deg | Heading accuracy (0 = heading not available). |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID 0x05. |
| `static bool decode(const message_view& raw, nav_att& out)` | `bool` | Decodes an exactly 32-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()`. |
| `roll_deg()`, `pitch_deg()`, `heading_deg()` | `double` | Angle × 1e-5. |
| `acc_roll_deg()`, `acc_pitch_deg()`, `acc_heading_deg()` | `double` | Accuracy × 1e-5. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_att.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_att;

void on_message(const message_view& raw)
{
    nav_att att{};
    if (nav_att::decode(raw, att) && (att.acc_heading != 0U))
    {
        const double heading = att.heading_deg();
        static_cast<void>(heading);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_att.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_att.cpp).

## Constraints & Notes

- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual).
- `decode()` accepts only a payload of exactly 32 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_deg()` helpers convert to `double` on demand.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
