# UBX MGA-GPS

## Overview

Models the five UBX-MGA-GPS (0x13 0x00) assistance layouts the host sends to the receiver: ephemeris, almanac, health, UTC and ionosphere. All share one class/ID and are told apart by payload byte 0 (`type`); each layout is its own `detail::typed_message`. The ephemeris and almanac layouts are templated on the message ID (`basic_mga_gps_eph<MsgId>`, `basic_mga_gps_alm<MsgId>`) because UBX-MGA-QZSS reuses them bit-for-bit (see [mga_qzss](mga_qzss.md)).

## Header

`#include "castle_ext/protocols/ubx/messages/mga_gps.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/typed_message.hpp`](detail/typed_message.md)
- `stdint.h`

## Public API

| Type | Base | Payload | `type` byte | Spec |
| --- | --- | --- | --- | --- |
| `basic_mga_gps_eph<MsgId>` / `mga_gps_eph` | `typed_message` | 68 B | `MGA_GPS_TYPE_EPH` (0x01) | 3.13.6.1 |
| `basic_mga_gps_alm<MsgId>` / `mga_gps_alm` | `typed_message` | 36 B | `MGA_GPS_TYPE_ALM` (0x02) | 3.13.6.2 |
| `mga_gps_health` | `typed_message` | 40 B | `MGA_GPS_TYPE_HEALTH` (0x04) | 3.13.6.3 |
| `mga_gps_utc` | `typed_message` | 20 B | `MGA_GPS_TYPE_UTC` (0x05) | 3.13.6.4 |
| `mga_gps_iono` | `typed_message` | 16 B | `MGA_GPS_TYPE_IONO` (0x06) | 3.13.6.5 |

| Constant | Value | Description |
| --- | --- | --- |
| `MGA_GPS_TYPE_EPH` .. `MGA_GPS_TYPE_IONO` | 0x01, 0x02, 0x04, 0x05, 0x06 | `type` bytes shared by MGA-GPS and MGA-QZSS. |
| `MGA_GPS_VERSION` | 0x00 | Message version of every layout. |
| `T::msg_class` / `T::msg_id` / `T::payload_length` / `T::message_type` | - | Per-model wire constants (`UBX_CLASS_MGA`, `UBX_ID_MGA_GPS` or the template ID, length, type byte). |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True only for this class/ID **and** exact payload length **and** payload byte 0 equal to the layout `type`. Exactly one layout of a class/ID matches any valid frame, so a decoder registry dispatches without ambiguity. |
| `static bool decode(const message_view&, T& out)` | Decodes the payload; returns `false` (and leaves `out` value-initialised) on wrong class/ID, wrong length, or a mismatching `type`/`version` byte. |
| `void encode(payload_writer&) const` | Writes the payload (type, version, fields, zeroed reserved bytes); used through `encode_message(msg, buf, cap, written)`. |

### `basic_mga_gps_eph<MsgId>` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `fit_interval` | `uint8_t` | `0U` | Fit interval flag. |
| `ura_index` | `uint8_t` | `0U` | User range accuracy. |
| `sv_health` | `uint8_t` | `0U` | SV health. |
| `tgd` | `int8_t` | `0` | Group delay differential [2^-31 s]. |
| `iodc` | `uint16_t` | `0U` | Issue of data, clock. |
| `toc` | `uint16_t` | `0U` | Clock data reference time [2^4 s]. |
| `af2` | `int8_t` | `0` | [2^-55 s/s^2]. |
| `af1` | `int16_t` | `0` | [2^-43 s/s]. |
| `af0` | `int32_t` | `0` | [2^-31 s]. |
| `crs` | `int16_t` | `0` | [2^-5 m]. |
| `delta_n` | `int16_t` | `0` | [2^-43 semi-circles/s]. |
| `m0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `cuc` | `int16_t` | `0` | [2^-29 rad]. |
| `cus` | `int16_t` | `0` | [2^-29 rad]. |
| `e` | `uint32_t` | `0U` | Eccentricity [2^-33]. |
| `sqrt_a` | `uint32_t` | `0U` | [2^-19 m^0.5]. |
| `toe` | `uint16_t` | `0U` | Ephemeris reference time [2^4 s]. |
| `cic` | `int16_t` | `0` | [2^-29 rad]. |
| `omega0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `cis` | `int16_t` | `0` | [2^-29 rad]. |
| `crc` | `int16_t` | `0` | [2^-5 m]. |
| `i0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `omega` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `omega_dot` | `int32_t` | `0` | [2^-43 semi-circles/s]. |
| `idot` | `int16_t` | `0` | [2^-43 semi-circles/s]. |

### `basic_mga_gps_alm<MsgId>` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `sv_health` | `uint8_t` | `0U` | SV health. |
| `e` | `uint16_t` | `0U` | Eccentricity [2^-21]. |
| `alm_wn_a` | `uint8_t` | `0U` | Almanac reference week (8-bit). |
| `toa` | `uint8_t` | `0U` | Almanac reference time [2^12 s]. |
| `delta_i` | `int16_t` | `0` | [2^-19 semi-circles]. |
| `omega_dot` | `int16_t` | `0` | [2^-38 semi-circles/s]. |
| `sqrt_a` | `uint32_t` | `0U` | [2^-11 m^0.5]. |
| `omega0` | `int32_t` | `0` | [2^-23 semi-circles]. |
| `omega` | `int32_t` | `0` | [2^-23 semi-circles]. |
| `m0` | `int32_t` | `0` | [2^-23 semi-circles]. |
| `af0` | `int16_t` | `0` | [2^-20 s]. |
| `af1` | `int16_t` | `0` | [2^-38 s/s]. |

### `mga_gps_health` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `health_code` | `castle::container::array<uint8_t, NUM_SVS>` | `{}` | `health_code[i]` is the 6-bit health of PRN `i + 1`. |

### `mga_gps_health` constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `NUM_SVS` | `castle::size_type` | `32U` | Number of `health_code` entries. |

### `mga_gps_utc` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `utc_a0` | `int32_t` | `0` | [2^-30 s]. |
| `utc_a1` | `int32_t` | `0` | [2^-50 s/s]. |
| `utc_dt_ls` | `int8_t` | `0` | Leap seconds before the event [s]. |
| `utc_tot` | `uint8_t` | `0U` | Reference time of week [2^12 s]. |
| `utc_wn_t` | `uint8_t` | `0U` | Reference week (8-bit). |
| `utc_wn_lsf` | `uint8_t` | `0U` | Leap second event week (8-bit). |
| `utc_dn` | `uint8_t` | `0U` | Leap second event day of week. |
| `utc_dt_lsf` | `int8_t` | `0` | Leap seconds after the event [s]. |

### `mga_gps_iono` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `iono_alpha` | `castle::container::array<int8_t, 4U>` | `{}` | alpha0..3 [2^-30, -27, -24, -24]. |
| `iono_beta` | `castle::container::array<int8_t, 4U>` | `{}` | beta0..3 [2^11, 14, 16, 16]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_gps.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

mga_gps_utc utc{};
utc.utc_dt_ls = 18;
uint8_t frame[mga_gps_utc::payload_length + UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status result = encode_message(utc, frame, sizeof(frame), written);

// Receiving side (loop-back / logging): only the UTC model claims this frame.
message_view raw{};
if (decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw) && mga_gps_utc::matches(raw)) {
    mga_gps_utc back{};
    static_cast<void>(mga_gps_utc::decode(raw, back));
}
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_gps.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Each layout requires its exact payload length; shorter or longer payloads are rejected before any field is read.
- The `type` byte is checked by `matches()` and `decode()`; the version byte (`0x00`) is checked by `decode()` through `stream.constant`, so a frame with an unknown version is claimed by its layout but rejected without partial state.
- Reserved bytes are skipped on decode and written as zero on encode.
- Scaled fields hold the raw wire integer; apply the documented scale (powers of two are left to the caller).
- `health_code[i]` is PRN `i + 1`; the spec places the 6-bit health code in each byte.
