# UBX NAV-POSLLH

## Overview

Typed model of UBX-NAV-POSLLH (class 0x01, ID 0x02), the periodic/polled geodetic position solution (longitude, latitude, ellipsoidal and MSL height with accuracy estimates) in the currently selected ellipsoid (WGS84 by default).

## Header

`#include "castle_ext/protocols/ubx/messages/nav_posllh.hpp"`

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
| `nav_posllh` | Value type for the payload; derives from `detail::fixed_message<nav_posllh, UBX_CLASS_NAV, UBX_ID_NAV_POSLLH, 28U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_posllh::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_posllh::msg_id` | `uint8_t` | `UBX_ID_NAV_POSLLH` | UBX message ID. |
| `nav_posllh::payload_length` | `castle::size_type` | `28U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `lon` | `int32_t` | 1e-7 deg | Longitude. |
| 8 | `lat` | `int32_t` | 1e-7 deg | Latitude. |
| 12 | `height` | `int32_t` | mm | Height above ellipsoid. |
| 16 | `h_msl` | `int32_t` | mm | Height above mean sea level. |
| 20 | `h_acc` | `uint32_t` | mm | Horizontal accuracy estimate. |
| 24 | `v_acc` | `uint32_t` | mm | Vertical accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_posllh::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_POSLLH`. |
| `static bool nav_posllh::decode(const message_view& raw, nav_posllh& out)` | `bool` | Decodes an exactly 28-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |
| `double longitude_deg() const` | `double` | `lon` × 1e-7. |
| `double latitude_deg() const` | `double` | `lat` × 1e-7. |
| `double height_m() const` | `double` | `height` × 1e-3. |
| `double h_msl_m() const` | `double` | `h_msl` × 1e-3. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_posllh.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_posllh;

void on_message(const message_view& raw)
{
    nav_posllh value{};
    if (nav_posllh::matches(raw) && nav_posllh::decode(raw, value))
    {
        const double lat = value.latitude_deg();
        const double lon = value.longitude_deg();
        const double alt = value.h_msl_m();
        static_cast<void>(lat); static_cast<void>(lon); static_cast<void>(alt);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_posllh.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_posllh.cpp).

## Constraints & Notes

- For sub-millimetre output use UBX-NAV-HPPOSLLH (`nav_hpposllh.hpp`).
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 28 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
