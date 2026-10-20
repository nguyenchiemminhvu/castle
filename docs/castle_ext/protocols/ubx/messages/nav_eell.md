# UBX NAV-EELL

## Overview

Typed model of UBX-NAV-EELL (class 0x01, ID 0x3D), the periodic/polled position error ellipse parameters. The 16-byte layout is shared with UBX-NAV2-EELL through one template, `basic_nav_eell<MsgClass, MsgId>`; this header defines `nav_eell`, and [`nav2_eell.hpp`](nav2_eell.md) defines `nav2_eell`.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"`

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
| `template <uint8_t MsgClass, uint8_t MsgId> basic_nav_eell` | Value type for the payload; derives from `detail::fixed_message<basic_nav_eell, MsgClass, MsgId, 16U>`. |
| `nav_eell` | `basic_nav_eell<UBX_CLASS_NAV, UBX_ID_NAV_EELL>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_eell::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_eell::msg_id` | `uint8_t` | `UBX_ID_NAV_EELL` (0x3D) | UBX message ID. |
| `nav_eell::payload_length` | `castle::size_type` | `16U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `version` | `uint8_t` | - | Message version (0x00 for this version). |
| 5 | - | `U1` | - | Reserved; skipped on decode, zero on encode. |
| 6 | `err_orient` | `uint16_t` | 1e-2 deg | `errEllipseOrient`: orientation of the semi-major axis, degrees from true north. |
| 8 | `err_maj` | `uint32_t` | mm | `errEllipseMajor`: semi-major axis of the error ellipse. |
| 12 | `err_min` | `uint32_t` | mm | `errEllipseMinor`: semi-minor axis of the error ellipse. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True when `raw` carries this class/ID. |
| `static bool decode(const message_view& raw, nav_eell& out)` | `bool` | Decodes an exactly 16-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved byte as zero); used by `encode_message()`. |
| `err_orient_deg()` | `double` | `err_orient` × 1e-2. |
| `err_maj_m()`, `err_min_m()` | `double` | Axis length × 1e-3. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_eell;

void on_message(const message_view& raw)
{
    nav_eell ellipse{};
    if (nav_eell::decode(raw, ellipse))
    {
        const double major_m = ellipse.err_maj_m();
        const double bearing = ellipse.err_orient_deg();
        static_cast<void>(major_m); static_cast<void>(bearing);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_eell.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_eell.cpp).

## Constraints & Notes

- Layout follows the ZED-F9R interface description: one reserved byte at offset 5, a `U2` orientation at offset 6, and `U4` axes at offsets 8 and 12. An earlier revision of this model read three `U2` values after a 3-byte gap; that layout was wrong and has been replaced, so `err_maj` / `err_min` are now `uint32_t`.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); `decode()` rejects any payload that is not exactly 16 bytes.
- Raw wire integers are stored unchanged; the helpers convert to `double` on demand.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
