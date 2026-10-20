# UBX NAV-DOP

## Overview

Typed model of UBX-NAV-DOP (class 0x01, ID 0x04), the periodic/polled dilution-of-precision values. All DOP values are dimensionless and scaled by 100 (156 on the wire = 1.56). The 18-byte layout is shared with UBX-NAV2-DOP through one template, `basic_nav_dop<MsgClass, MsgId>`; this header defines `nav_dop`, and [`nav2_dop.hpp`](nav2_dop.md) defines `nav2_dop`.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"`

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
| `template <uint8_t MsgClass, uint8_t MsgId> basic_nav_dop` | Value type for the payload; derives from `detail::fixed_message<basic_nav_dop, MsgClass, MsgId, 18U>`. |
| `nav_dop` | `basic_nav_dop<UBX_CLASS_NAV, UBX_ID_NAV_DOP>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_dop::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_dop::msg_id` | `uint8_t` | `UBX_ID_NAV_DOP` (0x04) | UBX message ID. |
| `nav_dop::payload_length` | `castle::size_type` | `18U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `g_dop` | `uint16_t` | 0.01 | Geometric DOP. |
| 6 | `p_dop` | `uint16_t` | 0.01 | Position DOP. |
| 8 | `t_dop` | `uint16_t` | 0.01 | Time DOP. |
| 10 | `v_dop` | `uint16_t` | 0.01 | Vertical DOP. |
| 12 | `h_dop` | `uint16_t` | 0.01 | Horizontal DOP. |
| 14 | `n_dop` | `uint16_t` | 0.01 | Northing DOP. |
| 16 | `e_dop` | `uint16_t` | 0.01 | Easting DOP. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True when `raw` carries this class/ID. |
| `static bool decode(const message_view& raw, nav_dop& out)` | `bool` | Decodes an exactly 18-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload; used by `encode_message()`. |
| `gdop_value()`, `pdop_value()`, `tdop_value()`, `vdop_value()`, `hdop_value()`, `ndop_value()`, `edop_value()` | `double` | Raw value × 0.01. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_dop;

bool geometry_good(const message_view& raw)
{
    nav_dop dop{};
    return nav_dop::decode(raw, dop) && (dop.pdop_value() < 2.5);
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_dop.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_dop.cpp).

## Constraints & Notes

- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual).
- `decode()` accepts only a payload of exactly 18 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_value()` helpers convert to `double` on demand.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
