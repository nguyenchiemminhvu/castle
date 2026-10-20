# UBX NAV2-EELL

## Overview

Typed model of UBX-NAV2-EELL (class 0x29, ID 0x3D), the secondary-output position error ellipse. Its payload is identical to UBX-NAV-EELL, so `nav2_eell` is an alias of the shared template `basic_nav_eell` with the NAV2 class/ID; fields and helpers are documented in [`nav_eell.md`](nav_eell.md).

## Header

`#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"`

## Dependencies

- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/messages/nav_eell.hpp`](nav_eell.md) (`basic_nav_eell`)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav2_eell` | `basic_nav_eell<UBX_CLASS_NAV2, UBX_ID_NAV2_EELL>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav2_eell::msg_class` | `uint8_t` | `UBX_CLASS_NAV2` (0x29) | UBX class. |
| `nav2_eell::msg_id` | `uint8_t` | `UBX_ID_NAV2_EELL` (0x3D) | UBX message ID. |
| `nav2_eell::payload_length` | `castle::size_type` | `16U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week. |
| 4 | `version` | `uint8_t` | - | Message version. |
| 5 | - | `U1` | - | Reserved. |
| 6 | `err_orient` | `uint16_t` | 1e-2 deg | Semi-major axis orientation from true north. |
| 8 | `err_maj` | `uint32_t` | mm | Semi-major axis. |
| 12 | `err_min` | `uint32_t` | mm | Semi-minor axis. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True only for class 0x29 / ID 0x3D (a NAV-EELL frame does not match). |
| `static bool decode(const message_view& raw, nav2_eell& out)` | `bool` | Decodes an exactly 16-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload; used by `encode_message()`. |
| `err_orient_deg()`, `err_maj_m()`, `err_min_m()` | `double` | Scaled helpers, see [`nav_eell.md`](nav_eell.md#functions). |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav2_eell;

double secondary_major_axis_m(const message_view& raw)
{
    nav2_eell ellipse{};
    return nav2_eell::decode(raw, ellipse) ? ellipse.err_maj_m() : -1.0;
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav2_eell.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav2_eell.cpp).

## Constraints & Notes

- `nav2_eell` and `nav_eell` are distinct types sharing one layout definition; `nav2_eell` no longer derives from `nav_eell`.
- The layout matches the spec (U2 orientation at 6, U4 axes at 8 and 12); the former three-`U2` layout was wrong.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); `decode()` rejects any payload that is not exactly 16 bytes.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
