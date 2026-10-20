# UBX MGA-INI

## Overview

Models the six UBX-MGA-INI (0x13 0x40) initial-assistance layouts sent by the host: ECEF and geodetic position, UTC and GNSS-time time, clock drift and external frequency. All share one class/ID and are told apart by payload byte 0 (`type`); layouts with the same length (POS_XYZ/POS_LLH, TIME_UTC/TIME_GNSS, CLKD/FREQ) are dispatched solely by that byte.

## Header

`#include "castle_ext/protocols/ubx/messages/mga_ini.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/typed_message.hpp`](detail/typed_message.md)
- `stdint.h`

## Public API

| Type | Base | Payload | `type` byte | Spec |
| --- | --- | --- | --- | --- |
| `mga_ini_pos_xyz` | `typed_message` | 20 B | `MGA_INI_TYPE_POS_XYZ` (0x00) | 3.13.7.1 |
| `mga_ini_pos_llh` | `typed_message` | 20 B | `MGA_INI_TYPE_POS_LLH` (0x01) | 3.13.7.2 |
| `mga_ini_time_utc` | `typed_message` | 24 B | `MGA_INI_TYPE_TIME_UTC` (0x10) | 3.13.7.3 |
| `mga_ini_time_gnss` | `typed_message` | 24 B | `MGA_INI_TYPE_TIME_GNSS` (0x11) | 3.13.7.4 |
| `mga_ini_clkd` | `typed_message` | 12 B | `MGA_INI_TYPE_CLKD` (0x20) | 3.13.7.5 |
| `mga_ini_freq` | `typed_message` | 12 B | `MGA_INI_TYPE_FREQ` (0x21) | 3.13.7.6 |

| Type / constant | Values | Description |
| --- | --- | --- |
| `MGA_INI_TYPE_*` | 0x00, 0x01, 0x10, 0x11, 0x20, 0x21 | Layout `type` bytes. |
| `MGA_INI_VERSION` | 0x00 | Message version of every layout. |
| `enum class mga_ini_time_source : uint8_t` | `none`=0, `extint0`=1, `extint1`=2 | `ref.source` of TIME_UTC / TIME_GNSS (3-15 reserved). |
| `enum class mga_ini_freq_source : uint8_t` | `extint0`=0, `extint1`=1 | `flags.source` of FREQ (2-15 reserved). |
| `enum class mga_ini_gnss_id : uint8_t` | `gps`=0, `galileo`=2, `beidou`=3, `glonass`=6 | TIME_GNSS time base. |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True only for this class/ID **and** exact payload length **and** payload byte 0 equal to the layout `type`. Exactly one layout of a class/ID matches any valid frame, so a decoder registry dispatches without ambiguity. |
| `static bool decode(const message_view&, T& out)` | Decodes the payload; returns `false` (and leaves `out` value-initialised) on wrong class/ID, wrong length, or a mismatching `type`/`version` byte. |
| `void encode(payload_writer&) const` | Writes the payload (type, version, fields, zeroed reserved bytes); used through `encode_message(msg, buf, cap, written)`. |

### `mga_ini_pos_xyz` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `ecef_x` | `int32_t` | `0` | [cm]. |
| `ecef_y` | `int32_t` | `0` | [cm]. |
| `ecef_z` | `int32_t` | `0` | [cm]. |
| `pos_acc` | `uint32_t` | `0U` | Position accuracy (stddev) [cm]. |

### `mga_ini_pos_xyz` helpers

| Function | Returns | Description |
| --- | --- | --- |
| `ecef_x_m()` | `double` | `ecef_x` in metres (`ecef_x · 1e-2`). |
| `ecef_y_m()` | `double` | `ecef_y` in metres. |
| `ecef_z_m()` | `double` | `ecef_z` in metres. |
| `pos_acc_m()` | `double` | `pos_acc` in metres. |

### `mga_ini_pos_llh` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `lat` | `int32_t` | `0` | Latitude [1e-7 deg]. |
| `lon` | `int32_t` | `0` | Longitude [1e-7 deg]. |
| `alt` | `int32_t` | `0` | Altitude [cm]. |
| `pos_acc` | `uint32_t` | `0U` | Position accuracy (stddev) [cm]. |

### `mga_ini_pos_llh` helpers

| Function | Returns | Description |
| --- | --- | --- |
| `latitude_deg()` | `double` | `lat · 1e-7` degrees. |
| `longitude_deg()` | `double` | `lon · 1e-7` degrees. |
| `alt_m()` | `double` | `alt` in metres. |
| `pos_acc_m()` | `double` | `pos_acc` in metres. |

### `mga_ini_time_utc` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `ref` | `uint8_t` | `0U` | Time reference (`REF_*` bits). |
| `leap_secs` | `int8_t` | `0` | Leap seconds since 1980, or `LEAP_SECS_UNKNOWN` [s]. |
| `year` | `uint16_t` | `0U` | Year. |
| `month` | `uint8_t` | `0U` | 1..12. |
| `day` | `uint8_t` | `0U` | 1..31. |
| `hour` | `uint8_t` | `0U` | 0..23. |
| `minute` | `uint8_t` | `0U` | 0..59. |
| `second` | `uint8_t` | `0U` | 0..59 [s]. |
| `ns` | `uint32_t` | `0U` | 0..999 999 999 [ns]. |
| `t_acc_s` | `uint16_t` | `0U` | Seconds part of time accuracy [s]. |
| `t_acc_ns` | `uint32_t` | `0U` | Nanoseconds part of time accuracy [ns]. |

### `mga_ini_time_utc` constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `REF_SOURCE` | `uint8_t` | `0x0FU` | `ref` bits 3..0: time source. |
| `REF_FALL` | `uint8_t` | `0x10U` | `ref` bit 4: falling EXTINT edge. |
| `REF_LAST` | `uint8_t` | `0x20U` | `ref` bit 5: last EXTINT pulse. |
| `LEAP_SECS_UNKNOWN` | `int8_t` | `-128` | `leap_secs` value meaning "unknown". |

### `mga_ini_time_utc` helpers

| Function | Returns | Description |
| --- | --- | --- |
| `source()` | `mga_ini_time_source` | Low nibble of `ref`/`flags` as the source enum. |
| `fall()` | `bool` | Use the falling EXTINT edge (default rising). |
| `last()` | `bool` | Use the last EXTINT pulse (default next pulse). |

### `mga_ini_time_gnss` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `ref` | `uint8_t` | `0U` | Time reference (`REF_*` bits). |
| `gnss_id` | `mga_ini_gnss_id` | `mga_ini_gnss_id::gps` | Time base. |
| `week` | `uint16_t` | `0U` | GNSS week number. |
| `tow` | `uint32_t` | `0U` | GNSS time of week [s]. |
| `ns` | `uint32_t` | `0U` | Sub-second part, 0..999 999 999 [ns]. |
| `t_acc_s` | `uint16_t` | `0U` | Seconds part of time accuracy [s]. |
| `t_acc_ns` | `uint32_t` | `0U` | Nanoseconds part of time accuracy [ns]. |

### `mga_ini_time_gnss` constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `REF_SOURCE` | `uint8_t` | `0x0FU` | `ref` bits 3..0: time source. |
| `REF_FALL` | `uint8_t` | `0x10U` | `ref` bit 4: falling EXTINT edge. |
| `REF_LAST` | `uint8_t` | `0x20U` | `ref` bit 5: last EXTINT pulse. |

### `mga_ini_time_gnss` helpers

| Function | Returns | Description |
| --- | --- | --- |
| `source()` | `mga_ini_time_source` | Low nibble of `ref`/`flags` as the source enum. |
| `fall()` | `bool` | Use the falling EXTINT edge (default rising). |
| `last()` | `bool` | Use the last EXTINT pulse (default next pulse). |

### `mga_ini_clkd` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `clk_d` | `int32_t` | `0` | Clock drift [ns/s]. |
| `clk_d_acc` | `uint32_t` | `0U` | Clock drift accuracy [ns/s]. |

### `mga_ini_freq` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `flags` | `uint8_t` | `0U` | Frequency reference (`FLAGS_*` bits). |
| `freq` | `int32_t` | `0` | Frequency [1e-2 Hz]. |
| `freq_acc` | `uint32_t` | `0U` | Frequency accuracy [ppb]. |

### `mga_ini_freq` constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `FLAGS_SOURCE` | `uint8_t` | `0x0FU` | `flags` bits 3..0: frequency source. |
| `FLAGS_FALL` | `uint8_t` | `0x10U` | `flags` bit 4: falling EXTINT edge. |

### `mga_ini_freq` helpers

| Function | Returns | Description |
| --- | --- | --- |
| `source()` | `mga_ini_freq_source` | Low nibble of `ref`/`flags` as the source enum. |
| `fall()` | `bool` | Use the falling EXTINT edge (default rising). |
| `freq_hz()` | `double` | `freq · 1e-2` Hz. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_ini.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

mga_ini_time_utc t{};
t.ref = static_cast<uint8_t>(mga_ini_time_source::extint0) | mga_ini_time_utc::REF_FALL;
t.leap_secs = mga_ini_time_utc::LEAP_SECS_UNKNOWN;
t.year = 2026U; t.month = 10U; t.day = 20U;
t.t_acc_s = 2U;
uint8_t frame[mga_ini_time_utc::payload_length + UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status result = encode_message(t, frame, sizeof(frame), written);
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_ini.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Each layout requires its exact payload length; shorter or longer payloads are rejected before any field is read.
- The `type` byte is checked by `matches()` and `decode()`; the version byte (`0x00`) is checked by `decode()` through `stream.constant`, so a frame with an unknown version is claimed by its layout but rejected without partial state.
- Reserved bytes are skipped on decode and written as zero on encode.
- Scaled fields hold the raw wire integer; apply the documented scale (powers of two are left to the caller).
- `ref` / `flags` are kept raw; `source()` returns the low nibble cast to the enum, so reserved source values (3-15 / 2-15) are passed through unchanged.
- Scaled helpers: `*_m()` converts centimetres to metres, `latitude_deg()`/`longitude_deg()` apply 1e-7, `freq_hz()` applies 1e-2.
