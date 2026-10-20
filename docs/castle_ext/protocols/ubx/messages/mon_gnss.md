# UBX MON-GNSS

## Overview

Model of UBX-MON-GNSS (class `0x0A`, ID `0x28`), the 8-byte polled report of the major GNSS selection: which constellations the receiver supports, enables by default and currently has enabled. Built on `detail::fixed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_gnss.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/fixed_message.hpp](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_gnss` | UBX-MON-GNSS payload (`fixed_message<mon_gnss, UBX_CLASS_MON, UBX_ID_MON_GNSS, 8U>`). |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x28` | Inherited from `fixed_message`. |
| `payload_length` | `8U` | Exact payload length. |
| `GNSS_GPS` | `0x01U` | Bit 0 of `supported` / `default_gnss` / `enabled` (`GPSSup`, `GPSDef`, `GPSEna`). |
| `GNSS_GLONASS` | `0x02U` | Bit 1 (`GlonassSup`, `GlonassDef`, `GlonassEna`). |
| `GNSS_BEIDOU` | `0x04U` | Bit 2 (`BeidouSup`, `BeidouDef`, `BeidouEna`). |
| `GNSS_GALILEO` | `0x08U` | Bit 3 (`GalileoSup`, `GalileoDef`, `GalileoEna`). |

### Fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | 0 | Message version (`0x01`, the default). |
| `supported` | `uint8_t` (X1) | 1 | Major GNSS the receiver can support. |
| `default_gnss` | `uint8_t` (X1) | 2 | Default major GNSS selection (efuse takes precedence over firmware). |
| `enabled` | `uint8_t` (X1) | 3 | Currently enabled major GNSS. |
| `simultaneous` | `uint8_t` | 4 | Maximum number of concurrent major GNSS. |
| — | reserved | 5..7 | Skipped / written as zero. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, mon_gnss&)` | Exact-length decode; `out` reset on failure. |
| `void encode(payload_writer&) const` | Writes the payload. |
| `bool is_supported(uint8_t gnss_mask) const` | `true` when every `GNSS_*` bit in `gnss_mask` is set in `supported`. |
| `bool is_default(uint8_t gnss_mask) const` | Same test against `default_gnss`. |
| `bool is_enabled(uint8_t gnss_mask) const` | Same test against `enabled`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_gnss.hpp"

using namespace castle::protocols::ubx;
using castle::protocols::ubx::messages::mon_gnss;

uint8_t poll[UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status sent = encode_poll<mon_gnss>(poll, sizeof(poll), written);

// ... later, for the answer:
mon_gnss gnss{};
if (mon_gnss::decode(raw, gnss) && gnss.is_enabled(mon_gnss::GNSS_GALILEO))
{
}
```

See `samples/castle_ext/protocols/ubx/messages/mon_gnss.cpp`.

## Constraints & Notes

- Value type: every field is stored in the object itself; no heap allocation, no exceptions.
- `decode()` rejects a different class/ID or any payload length other than the exact 8 bytes and leaves `out` value-initialised on failure.
- Reserved bytes are skipped on decode and written as zero on encode.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none; not thread-safe. Use one instance per thread or synchronise externally.
- Polled: request it with `encode_poll<mon_gnss>()` (empty payload); no separate poll struct exists.
- Augmentation systems (SBAS, QZSS) are not reported by this message.
