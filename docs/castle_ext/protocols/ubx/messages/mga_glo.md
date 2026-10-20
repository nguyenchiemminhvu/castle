# UBX MGA-GLO

## Overview

Models the three UBX-MGA-GLO (0x13 0x06) GLONASS assistance layouts sent by the host: ephemeris, almanac and auxiliary time offsets. All share one class/ID and are told apart by payload byte 0 (`type`); each layout is its own `detail::typed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/mga_glo.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/typed_message.hpp`](detail/typed_message.md)
- `stdint.h`

## Public API

| Type | Base | Payload | `type` byte | Spec |
| --- | --- | --- | --- | --- |
| `mga_glo_eph` | `typed_message` | 48 B | `MGA_GLO_TYPE_EPH` (0x01) | 3.13.5.1 |
| `mga_glo_alm` | `typed_message` | 36 B | `MGA_GLO_TYPE_ALM` (0x02) | 3.13.5.2 |
| `mga_glo_timeoffset` | `typed_message` | 20 B | `MGA_GLO_TYPE_TIMEOFFSET` (0x03) | 3.13.5.3 |

| Constant | Value | Description |
| --- | --- | --- |
| `MGA_GLO_TYPE_EPH` .. `MGA_GLO_TYPE_TIMEOFFSET` | 0x01, 0x02, 0x03 | Layout `type` bytes. |
| `MGA_GLO_VERSION` | 0x00 | Message version of every layout. |
| `MGA_GLO_FREQ_CHANNEL_UNKNOWN` | -128 | `mga_glo_eph::h` value for an unknown frequency channel. |
| `T::msg_class` / `T::msg_id` / `T::payload_length` / `T::message_type` | - | Per-model wire constants. |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True only for this class/ID **and** exact payload length **and** payload byte 0 equal to the layout `type`. Exactly one layout of a class/ID matches any valid frame, so a decoder registry dispatches without ambiguity. |
| `static bool decode(const message_view&, T& out)` | Decodes the payload; returns `false` (and leaves `out` value-initialised) on wrong class/ID, wrong length, or a mismatching `type`/`version` byte. |
| `void encode(payload_writer&) const` | Writes the payload (type, version, fields, zeroed reserved bytes); used through `encode_message(msg, buf, cap, written)`. |

### `mga_glo_eph` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `ft` | `uint8_t` | `0U` | User range accuracy. |
| `b` | `uint8_t` | `0U` | Health flag from string 2. |
| `m` | `uint8_t` | `0U` | Satellite type (1 = GLONASS-M). |
| `h` | `int8_t` | `0` | Frequency channel number (-7..6, -128 = unknown). |
| `x` | `int32_t` | `0` | PZ-90.02 position X [2^-11 km]. |
| `y` | `int32_t` | `0` | PZ-90.02 position Y [2^-11 km]. |
| `z` | `int32_t` | `0` | PZ-90.02 position Z [2^-11 km]. |
| `dx` | `int32_t` | `0` | PZ-90.02 velocity X [2^-20 km/s]. |
| `dy` | `int32_t` | `0` | PZ-90.02 velocity Y [2^-20 km/s]. |
| `dz` | `int32_t` | `0` | PZ-90.02 velocity Z [2^-20 km/s]. |
| `ddx` | `int8_t` | `0` | PZ-90.02 acceleration X [2^-30 km/s^2]. |
| `ddy` | `int8_t` | `0` | PZ-90.02 acceleration Y [2^-30 km/s^2]. |
| `ddz` | `int8_t` | `0` | PZ-90.02 acceleration Z [2^-30 km/s^2]. |
| `tb` | `uint8_t` | `0U` | Time-interval index within the UTC(SU) day [15 min]. |
| `gamma` | `int16_t` | `0` | Relative carrier frequency deviation [2^-40]. |
| `e` | `uint8_t` | `0U` | Ephemeris data age [day]. |
| `delta_tau` | `int8_t` | `0` | L2-L1 time difference [2^-30 s]. |
| `tau` | `int32_t` | `0` | SV clock bias [2^-30 s]. |

### `mga_glo_alm` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier (see Satellite Numbering). |
| `n` | `uint16_t` | `0U` | Almanac calendar day within the four-year period [day]. |
| `m` | `uint8_t` | `0U` | Satellite type (1 = GLONASS-M). |
| `c` | `uint8_t` | `0U` | Operability flag at almanac upload (1 = operable). |
| `tau` | `int16_t` | `0` | Coarse time correction to GLONASS time [2^-18 s]. |
| `epsilon` | `uint16_t` | `0U` | Eccentricity [2^-20]. |
| `lambda` | `int32_t` | `0` | Longitude of first ascending node [2^-20 semi-circles]. |
| `delta_i` | `int32_t` | `0` | Inclination correction [2^-20 semi-circles]. |
| `t_lambda` | `uint32_t` | `0U` | Time of first ascending node passage [2^-5 s]. |
| `delta_t` | `int32_t` | `0` | Draconian period correction [2^-9 s/orbital-period]. |
| `delta_dt` | `int8_t` | `0` | Draconian period rate [2^-14 s/orbital-period^2]. |
| `h` | `int8_t` | `0` | Frequency channel number (-7..6). |
| `omega` | `int16_t` | `0` | Argument of perigee (scale not given by the spec). |

### `mga_glo_timeoffset` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `n` | `uint16_t` | `0U` | Almanac calendar day within the four-year period [day]. |
| `tau_c` | `int32_t` | `0` | Time scale correction to UTC(SU) [2^-27 s]. |
| `tau_gps` | `int32_t` | `0` | GPS time relative to GLONASS time [2^-31 s]. |
| `b1` | `int16_t` | `0` | Delta UT1 coefficient [2^-10 s]. |
| `b2` | `int16_t` | `0` | Delta UT1 rate [2^-16 s/msd]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_glo.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

mga_glo_eph eph{};
eph.sv_id = 4U;
eph.h = MGA_GLO_FREQ_CHANNEL_UNKNOWN;
uint8_t frame[mga_glo_eph::payload_length + UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status result = encode_message(eph, frame, sizeof(frame), written);
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_glo.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Each layout requires its exact payload length; shorter or longer payloads are rejected before any field is read.
- The `type` byte is checked by `matches()` and `decode()`; the version byte (`0x00`) is checked by `decode()` through `stream.constant`, so a frame with an unknown version is claimed by its layout but rejected without partial state.
- Reserved bytes are skipped on decode and written as zero on encode.
- Scaled fields hold the raw wire integer; apply the documented scale (powers of two are left to the caller).
- Single-letter spec names are lower-cased (`FT` -> `ft`, `B` -> `b`, `M` -> `m`, `H` -> `h`, `E` -> `e`, `N` -> `n`, `C` -> `c`, `B1`/`B2` -> `b1`/`b2`).
- The spec gives no scale for `mga_glo_alm::omega`; it is exposed as the raw I2.
