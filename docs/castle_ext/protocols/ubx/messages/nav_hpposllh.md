# UBX NAV-HPPOSLLH

## Overview

Typed model of UBX-NAV-HPPOSLLH (class 0x01, ID 0x14), the periodic/polled high-precision geodetic position. Angles combine a 1e-7 deg part with a 1e-9 deg part (lon + lonHp × 1e-2 in 1e-7 deg); heights combine a mm part with a 0.1 mm part (height + heightHp × 0.1 in mm).

## Header

`#include "castle_ext/protocols/ubx/messages/nav_hpposllh.hpp"`

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
| `nav_hpposllh` | Value type for the payload; derives from `detail::fixed_message<nav_hpposllh, UBX_CLASS_NAV, UBX_ID_NAV_HPPOSLLH, 36U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_hpposllh::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_hpposllh::msg_id` | `uint8_t` | `UBX_ID_NAV_HPPOSLLH` | UBX message ID. |
| `nav_hpposllh::payload_length` | `castle::size_type` | `36U` | Exact payload length. |
| `nav_hpposllh::FLAGS_INVALID_LLH` | `uint8_t` | `0x01U` | `flags` bit 0 `invalidLlh`: lon/lat/height/hMSL and HP parts invalid. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `version` | `uint8_t` | - | Message version (0x00 for this layout); stored, not validated. |
| 1 | - | `U1[2]` | - | Reserved; skipped on decode, written as zero. |
| 3 | `flags` | `uint8_t` | bitfield | Additional flags, see `FLAGS_*`. |
| 4 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 8 | `lon` | `int32_t` | 1e-7 deg | Longitude. |
| 12 | `lat` | `int32_t` | 1e-7 deg | Latitude. |
| 16 | `height` | `int32_t` | mm | Height above ellipsoid. |
| 20 | `h_msl` | `int32_t` | mm | Height above mean sea level. |
| 24 | `lon_hp` | `int8_t` | 1e-9 deg | High-precision part of longitude, -99..+99. |
| 25 | `lat_hp` | `int8_t` | 1e-9 deg | High-precision part of latitude, -99..+99. |
| 26 | `height_hp` | `int8_t` | 0.1 mm | High-precision part of height, -9..+9. |
| 27 | `h_msl_hp` | `int8_t` | 0.1 mm | High-precision part of hMSL, -9..+9. |
| 28 | `h_acc` | `uint32_t` | 0.1 mm | Horizontal accuracy estimate. |
| 32 | `v_acc` | `uint32_t` | 0.1 mm | Vertical accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_hpposllh::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_HPPOSLLH`. |
| `static bool nav_hpposllh::decode(const message_view& raw, nav_hpposllh& out)` | `bool` | Decodes an exactly 36-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |
| `bool invalid_llh() const` | `bool` | `flags` bit 0 set. |
| `double longitude_deg() const` | `double` | `lon` × 1e-7 + `lon_hp` × 1e-9 [deg]. |
| `double latitude_deg() const` | `double` | `lat` × 1e-7 + `lat_hp` × 1e-9 [deg]. |
| `double height_m() const` | `double` | `height` × 1e-3 + `height_hp` × 1e-4 [m]. |
| `double h_msl_m() const` | `double` | `h_msl` × 1e-3 + `h_msl_hp` × 1e-4 [m]. |
| `double h_acc_m() const` | `double` | `h_acc` × 1e-4 [m]. |
| `double v_acc_m() const` | `double` | `v_acc` × 1e-4 [m]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_hpposllh.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_hpposllh;

void on_message(const message_view& raw)
{
    nav_hpposllh value{};
    if (nav_hpposllh::matches(raw) && nav_hpposllh::decode(raw, value))
    {
        if (!value.invalid_llh())
        {
            const double lat = value.latitude_deg(); // 1e-7 + 1e-9 deg parts combined
            static_cast<void>(lat);
        }
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_hpposllh.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_hpposllh.cpp).

## Constraints & Notes

- Unlike NAV-POSLLH, `h_acc`/`v_acc` are in 0.1 mm here (spec scale 0.1).
- HP part ranges (-99..+99, -9..+9) are documented by the spec but not enforced.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 36 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
