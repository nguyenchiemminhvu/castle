# UBX MGA-BDS

## Overview

Models the five UBX-MGA-BDS (0x13 0x03) BeiDou assistance layouts sent by the host: ephemeris, almanac, health, UTC and ionosphere. All share one class/ID and are told apart by payload byte 0 (`type`); each layout is its own `detail::typed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/mga_bds.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/typed_message.hpp`](detail/typed_message.md)
- `stdint.h`

## Public API

| Type | Base | Payload | `type` byte | Spec |
| --- | --- | --- | --- | --- |
| `mga_bds_eph` | `typed_message` | 88 B | `MGA_BDS_TYPE_EPH` (0x01) | 3.13.2.1 |
| `mga_bds_alm` | `typed_message` | 40 B | `MGA_BDS_TYPE_ALM` (0x02) | 3.13.2.2 |
| `mga_bds_health` | `typed_message` | 68 B | `MGA_BDS_TYPE_HEALTH` (0x04) | 3.13.2.3 |
| `mga_bds_utc` | `typed_message` | 20 B | `MGA_BDS_TYPE_UTC` (0x05) | 3.13.2.4 |
| `mga_bds_iono` | `typed_message` | 16 B | `MGA_BDS_TYPE_IONO` (0x06) | 3.13.2.5 |

| Constant | Value | Description |
| --- | --- | --- |
| `MGA_BDS_TYPE_EPH` .. `MGA_BDS_TYPE_IONO` | 0x01, 0x02, 0x04, 0x05, 0x06 | Layout `type` bytes. |
| `MGA_BDS_VERSION` | 0x00 | Message version of every layout. |
| `T::msg_class` / `T::msg_id` / `T::payload_length` / `T::message_type` | - | Per-model wire constants. |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True only for this class/ID **and** exact payload length **and** payload byte 0 equal to the layout `type`. Exactly one layout of a class/ID matches any valid frame, so a decoder registry dispatches without ambiguity. |
| `static bool decode(const message_view&, T& out)` | Decodes the payload; returns `false` (and leaves `out` value-initialised) on wrong class/ID, wrong length, or a mismatching `type`/`version` byte. |
| `void encode(payload_writer&) const` | Writes the payload (type, version, fields, zeroed reserved bytes); used through `encode_message(msg, buf, cap, written)`. |

### `mga_bds_eph` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `sat_h1` | `uint8_t` | `0U` | Autonomous satellite health flag. |
| `iodc` | `uint8_t` | `0U` | Issue of data, clock. |
| `a2` | `int16_t` | `0` | Time polynomial coefficient 2 [2^-66 s/s^2]. |
| `a1` | `int32_t` | `0` | Time polynomial coefficient 1 [2^-50 s/s]. |
| `a0` | `int32_t` | `0` | Time polynomial coefficient 0 [2^-33 s]. |
| `toc` | `uint32_t` | `0U` | Clock data reference time [2^3 s]. |
| `tgd1` | `int16_t` | `0` | Equipment group delay differential [0.1 ns]. |
| `urai` | `uint8_t` | `0U` | User range accuracy index. |
| `iode` | `uint8_t` | `0U` | Issue of data, ephemeris. |
| `toe` | `uint32_t` | `0U` | Ephemeris reference time [2^3 s]. |
| `sqrt_a` | `uint32_t` | `0U` | [2^-19 m^0.5]. |
| `e` | `uint32_t` | `0U` | Eccentricity [2^-33]. |
| `omega` | `int32_t` | `0` | Argument of perigee [2^-31 semi-circles]. |
| `delta_n` | `int16_t` | `0` | [2^-43 semi-circles/s]. |
| `idot` | `int16_t` | `0` | [2^-43 semi-circles/s]. |
| `m0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `omega0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `omega_dot` | `int32_t` | `0` | [2^-43 semi-circles/s]. |
| `i0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `cuc` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `cus` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `crc` | `int32_t` | `0` | [2^-6 m]. |
| `crs` | `int32_t` | `0` | [2^-6 m]. |
| `cic` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `cis` | `int32_t` | `0` | [2^-31 semi-circles]. |

### `mga_bds_alm` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `wna` | `uint8_t` | `0U` | Almanac week number [week]. |
| `toa` | `uint8_t` | `0U` | Almanac reference time [2^12 s]. |
| `delta_i` | `int16_t` | `0` | [2^-19 semi-circles]. |
| `sqrt_a` | `uint32_t` | `0U` | [2^-11 m^0.5]. |
| `e` | `uint32_t` | `0U` | Eccentricity [2^-21]. |
| `omega` | `int32_t` | `0` | Argument of perigee [2^-23 semi-circles]. |
| `m0` | `int32_t` | `0` | [2^-23 semi-circles]. |
| `omega0` | `int32_t` | `0` | [2^-23 semi-circles]. |
| `omega_dot` | `int32_t` | `0` | [2^-38 semi-circles/s]. |
| `a0` | `int16_t` | `0` | Satellite clock bias [2^-20 s]. |
| `a1` | `int16_t` | `0` | Satellite clock rate [2^-38 s/s]. |

### `mga_bds_health` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `health_code` | `castle::container::array<uint16_t, NUM_SVS>` | `{}` | `health_code[i]` is the health word of SV `i + 1`. |

### `mga_bds_health` constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `NUM_SVS` | `castle::size_type` | `30U` | Number of `health_code` entries. |
| `HEALTH_CODE_MASK` | `uint16_t` | `0x01FFU` | The 9 LSBs of each `health_code` entry carry the D1 health code. |

### `mga_bds_utc` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `a0_utc` | `int32_t` | `0` | BDT clock bias relative to UTC [2^-30 s]. |
| `a1_utc` | `int32_t` | `0` | BDT clock rate relative to UTC [2^-50 s/s]. |
| `dt_ls` | `int8_t` | `0` | Leap seconds before the new leap second [s]. |
| `wn_rec` | `uint8_t` | `0U` | Week of reception of this parameter set (8-bit) [week]. |
| `wn_lsf` | `uint8_t` | `0U` | Week of the new leap second [week]. |
| `dn` | `uint8_t` | `0U` | Day number of the new leap second [day]. |
| `dt_lsf` | `int8_t` | `0` | Leap seconds after the new leap second [s]. |

### `mga_bds_iono` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `alpha` | `castle::container::array<int8_t, 4U>` | `{}` | alpha0..3 [2^-30, -27, -24, -24]. |
| `beta` | `castle::container::array<int8_t, 4U>` | `{}` | beta0..3 [2^11, 14, 16, 16]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_bds.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

mga_bds_health health{};
health.health_code[0U] = 0x0000U; // SV 1 healthy
uint8_t frame[mga_bds_health::payload_length + UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status result = encode_message(health, frame, sizeof(frame), written);
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_bds.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Each layout requires its exact payload length; shorter or longer payloads are rejected before any field is read.
- The `type` byte is checked by `matches()` and `decode()`; the version byte (`0x00`) is checked by `decode()` through `stream.constant`, so a frame with an unknown version is claimed by its layout but rejected without partial state.
- Reserved bytes are skipped on decode and written as zero on encode.
- Scaled fields hold the raw wire integer; apply the documented scale (powers of two are left to the caller).
- `mga_bds_health::health_code` is `U2[30]` on the wire (SV `i + 1`); only the 9 LSBs (`HEALTH_CODE_MASK`) carry the D1 health code, the raw word is kept as received.
