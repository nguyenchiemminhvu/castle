# UBX MGA-QZSS

## Overview

Models the three UBX-MGA-QZSS (0x13 0x05) assistance layouts sent by the host. The ephemeris and almanac layouts are identical to GPS, so they are aliases of the `mga_gps.hpp` templates instantiated on `UBX_ID_MGA_QZSS`; only the 5-SV health layout is QZSS-specific. The `type`/version bytes are the shared `MGA_GPS_TYPE_*` / `MGA_GPS_VERSION` constants.

## Header

`#include "castle_ext/protocols/ubx/messages/mga_qzss.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/typed_message.hpp`](detail/typed_message.md)
- [`castle_ext/protocols/ubx/messages/mga_gps.hpp`](mga_gps.md)
- `stdint.h`

## Public API

| Type | Base | Payload | `type` byte | Spec |
| --- | --- | --- | --- | --- |
| `mga_qzss_eph` = `basic_mga_gps_eph<UBX_ID_MGA_QZSS>` | `typed_message` | 68 B | `MGA_GPS_TYPE_EPH` (0x01) | 3.13.8.1 |
| `mga_qzss_alm` = `basic_mga_gps_alm<UBX_ID_MGA_QZSS>` | `typed_message` | 36 B | `MGA_GPS_TYPE_ALM` (0x02) | 3.13.8.2 |
| `mga_qzss_health` | `typed_message` | 12 B | `MGA_GPS_TYPE_HEALTH` (0x04) | 3.13.8.3 |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True only for this class/ID **and** exact payload length **and** payload byte 0 equal to the layout `type`. Exactly one layout of a class/ID matches any valid frame, so a decoder registry dispatches without ambiguity. |
| `static bool decode(const message_view&, T& out)` | Decodes the payload; returns `false` (and leaves `out` value-initialised) on wrong class/ID, wrong length, or a mismatching `type`/`version` byte. |
| `void encode(payload_writer&) const` | Writes the payload (type, version, fields, zeroed reserved bytes); used through `encode_message(msg, buf, cap, written)`. |

The fields of `mga_qzss_eph` / `mga_qzss_alm` are documented in [mga_gps](mga_gps.md).

### `mga_qzss_health` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `health_code` | `castle::container::array<uint8_t, NUM_SVS>` | `{}` | `health_code[i]` is the health of QZSS SV `i + 1`. |

### `mga_qzss_health` constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `NUM_SVS` | `castle::size_type` | `5U` | Number of `health_code` entries. |
| `HEALTH_CODE_MASK` | `uint8_t` | `0x3FU` | The 6 LSBs of each `health_code` entry carry the health code. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_qzss.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

mga_qzss_eph eph{};          // same fields as mga_gps_eph, encoded with ID 0x05
eph.sv_id = 194U;
uint8_t frame[mga_qzss_eph::payload_length + UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status result = encode_message(eph, frame, sizeof(frame), written);
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_qzss.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Each layout requires its exact payload length; shorter or longer payloads are rejected before any field is read.
- The `type` byte is checked by `matches()` and `decode()`; the version byte (`0x00`) is checked by `decode()` through `stream.constant`, so a frame with an unknown version is claimed by its layout but rejected without partial state.
- Reserved bytes are skipped on decode and written as zero on encode.
- Scaled fields hold the raw wire integer; apply the documented scale (powers of two are left to the caller).
- A GPS-ID frame never matches a QZSS model (and vice versa) even though the layouts are identical, because `matches()` checks the message ID.
- `health_code[i]` is QZSS SV `i + 1`; only the 6 LSBs (`HEALTH_CODE_MASK`) carry the health code.
