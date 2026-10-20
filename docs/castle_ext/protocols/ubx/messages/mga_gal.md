# UBX MGA-GAL

## Overview

Models the four UBX-MGA-GAL (0x13 0x02) Galileo assistance layouts sent by the host: ephemeris, almanac, Galileo-GPS time offset (GGTO) and UTC. All share one class/ID and are told apart by payload byte 0 (`type`); each layout is its own `detail::typed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/mga_gal.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/typed_message.hpp`](detail/typed_message.md)
- `stdint.h`

## Public API

| Type | Base | Payload | `type` byte | Spec |
| --- | --- | --- | --- | --- |
| `mga_gal_eph` | `typed_message` | 76 B | `MGA_GAL_TYPE_EPH` (0x01) | 3.13.4.1 |
| `mga_gal_alm` | `typed_message` | 32 B | `MGA_GAL_TYPE_ALM` (0x02) | 3.13.4.2 |
| `mga_gal_timeoffset` | `typed_message` | 12 B | `MGA_GAL_TYPE_TIMEOFFSET` (0x03) | 3.13.4.3 |
| `mga_gal_utc` | `typed_message` | 20 B | `MGA_GAL_TYPE_UTC` (0x05) | 3.13.4.4 |

| Constant | Value | Description |
| --- | --- | --- |
| `MGA_GAL_TYPE_EPH` .. `MGA_GAL_TYPE_UTC` | 0x01, 0x02, 0x03, 0x05 | Layout `type` bytes. |
| `MGA_GAL_VERSION` | 0x00 | Message version of every layout. |
| `T::msg_class` / `T::msg_id` / `T::payload_length` / `T::message_type` | - | Per-model wire constants. |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True only for this class/ID **and** exact payload length **and** payload byte 0 equal to the layout `type`. Exactly one layout of a class/ID matches any valid frame, so a decoder registry dispatches without ambiguity. |
| `static bool decode(const message_view&, T& out)` | Decodes the payload; returns `false` (and leaves `out` value-initialised) on wrong class/ID, wrong length, or a mismatching `type`/`version` byte. |
| `void encode(payload_writer&) const` | Writes the payload (type, version, fields, zeroed reserved bytes); used through `encode_message(msg, buf, cap, written)`. |

### `mga_gal_eph` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `iod_nav` | `uint16_t` | `0U` | Ephemeris and clock correction issue of data. |
| `delta_n` | `int16_t` | `0` | [2^-43 semi-circles/s]. |
| `m0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `e` | `uint32_t` | `0U` | Eccentricity [2^-33]. |
| `sqrt_a` | `uint32_t` | `0U` | [2^-19 m^0.5]. |
| `omega0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `i0` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `omega` | `int32_t` | `0` | [2^-31 semi-circles]. |
| `omega_dot` | `int32_t` | `0` | [2^-43 semi-circles/s]. |
| `i_dot` | `int16_t` | `0` | [2^-43 semi-circles/s]. |
| `cuc` | `int16_t` | `0` | [2^-29 rad]. |
| `cus` | `int16_t` | `0` | [2^-29 rad]. |
| `crc` | `int16_t` | `0` | [2^-5 m] (the spec table prints "radians"). |
| `crs` | `int16_t` | `0` | [2^-5 m] (the spec table prints "radians"). |
| `cic` | `int16_t` | `0` | [2^-29 rad]. |
| `cis` | `int16_t` | `0` | [2^-29 rad]. |
| `toe` | `uint16_t` | `0U` | Ephemeris reference time [60 s]. |
| `af0` | `int32_t` | `0` | [2^-34 s]. |
| `af1` | `int32_t` | `0` | [2^-46 s/s]. |
| `af2` | `int8_t` | `0` | [2^-59 s/s^2]. |
| `sisa_index_e1_e5b` | `uint8_t` | `0U` | Signal-in-space accuracy index (E1-E5b). |
| `toc` | `uint16_t` | `0U` | Clock correction reference time of week [60 s]. |
| `bgd_e1_e5b` | `int16_t` | `0` | E1-E5b broadcast group delay. |
| `health_e1b` | `uint8_t` | `0U` | E1-B signal health status. |
| `data_validity_e1b` | `uint8_t` | `0U` | E1-B data validity status. |
| `health_e5b` | `uint8_t` | `0U` | E5b signal health status. |
| `data_validity_e5b` | `uint8_t` | `0U` | E5b data validity status. |

### `mga_gal_alm` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `ioda` | `uint8_t` | `0U` | Almanac issue of data. |
| `alm_wn_a` | `uint8_t` | `0U` | Almanac reference week number [week]. |
| `toa` | `uint16_t` | `0U` | Almanac reference time [600 s]. |
| `delta_sqrt_a` | `int16_t` | `0` | Delta to sqrt of nominal semi-major axis (29 600 km) [2^-9 m^0.5]. |
| `e` | `uint16_t` | `0U` | Eccentricity [2^-16]. |
| `delta_i` | `int16_t` | `0` | Inclination relative to 56 deg [2^-14 semi-circles]. |
| `omega0` | `int16_t` | `0` | [2^-15 semi-circles]. |
| `omega_dot` | `int16_t` | `0` | [2^-33 semi-circles/s]. |
| `omega` | `int16_t` | `0` | [2^-15 semi-circles]. |
| `m0` | `int16_t` | `0` | [2^-15 semi-circles]. |
| `af0` | `int16_t` | `0` | [2^-19 s]. |
| `af1` | `int16_t` | `0` | [2^-38 s/s]. |
| `health_e1b` | `uint8_t` | `0U` | E1-B signal health status. |
| `health_e5b` | `uint8_t` | `0U` | E5b signal health status. |

### `mga_gal_timeoffset` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `a0_g` | `int16_t` | `0` | Constant term of the offset polynomial [2^-35 s]. |
| `a1_g` | `int16_t` | `0` | Rate of change of the offset [2^-51 s/s]. |
| `t0_g` | `uint8_t` | `0U` | GGTO reference time [3600 s]. |
| `wn0_g` | `uint8_t` | `0U` | GGTO reference week number [week]. |

### `mga_gal_utc` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `a0` | `int32_t` | `0` | First UTC polynomial parameter [2^-30 s]. |
| `a1` | `int32_t` | `0` | Second UTC polynomial parameter [2^-50 s/s]. |
| `dt_ls` | `int8_t` | `0` | Current leap seconds [s]. |
| `tot` | `uint8_t` | `0U` | UTC reference time of week (Galileo time) [3600 s]. |
| `wnt` | `uint8_t` | `0U` | UTC reference week number (8-bit) [week]. |
| `wn_lsf` | `uint8_t` | `0U` | Week of the future leap second (8-bit) [week]. |
| `dn` | `uint8_t` | `0U` | Day of the future leap second [day]. |
| `dt_lsf` | `int8_t` | `0` | Future leap seconds (spec `dTLSF`) [s]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_gal.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

mga_gal_timeoffset ggto{};
ggto.a0_g = -12;
ggto.t0_g = 24U;
uint8_t frame[mga_gal_timeoffset::payload_length + UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status result = encode_message(ggto, frame, sizeof(frame), written);
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_gal.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Each layout requires its exact payload length; shorter or longer payloads are rejected before any field is read.
- The `type` byte is checked by `matches()` and `decode()`; the version byte (`0x00`) is checked by `decode()` through `stream.constant`, so a frame with an unknown version is claimed by its layout but rejected without partial state.
- Reserved bytes are skipped on decode and written as zero on encode.
- Scaled fields hold the raw wire integer; apply the documented scale (powers of two are left to the caller).
- Spec field names ending in a continuation line are joined: `sisaIndexE1E5b` -> `sisa_index_e1_e5b`, `dataValidityE1B` -> `data_validity_e1b`, `dataValidityE5b` -> `data_validity_e5b`, `dTLSF` -> `dt_lsf`.
- The spec table gives `crc`/`crs` the unit "radians"; they are orbit-radius corrections in metres (2^-5 m), as for GPS.
