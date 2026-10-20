# UBX NAV-SLAS

## Overview

`nav_slas<MaxCorrections>` models UBX-NAV-SLAS (0x01 0x42), the periodic/polled QZSS L1S SLAS status data: a 20-byte header followed by `cnt` 8-byte pseudorange-correction blocks (`20 + cnt·8`). The header identifies the ground monitoring station (position and code), the QZSS SV supplying corrections and the service flags.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_slas.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/repeated_group.hpp`](detail/repeated_group.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_slas_correction` | One 8-byte correction block (`gnssId`, `svId`, `reserved1` U1, `reserved2` U1[3], `prc`). |
| `nav_slas<MaxCorrections = 16U>` | The message; `MaxCorrections <= 255`. |

### Constants

| Constant | Value |
| --- | --- |
| `nav_slas::msg_class` / `msg_id` | `UBX_CLASS_NAV` / `UBX_ID_NAV_SLAS` |
| `nav_slas::header_length` | `20U` |
| `nav_slas::block_length` / `nav_slas_correction::wire_size` | `8U` |
| `nav_slas::SERVICE_FLAGS_GMS_AVAILABLE` | `0x01` (bit 0) |
| `nav_slas::SERVICE_FLAGS_QZSS_SV_AVAILABLE` | `0x02` (bit 1) |
| `nav_slas::SERVICE_FLAGS_TEST_MODE` | `0x04` (bit 2) |

### `nav_slas` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| `version` | `uint8_t` | - | Message version (0x00). |
| `gms_lon` | `int32_t` | 1e-3 deg | Ground monitoring station longitude. |
| `gms_lat` | `int32_t` | 1e-3 deg | Ground monitoring station latitude. |
| `gms_code` | `uint8_t` | - | Ground monitoring station code (QZSS SLAS IS). |
| `qzss_sv_id` | `uint8_t` | - | QZS/GEO whose correction data is used. |
| `service_flags` | `uint8_t` | - | SLAS service flags (`SERVICE_FLAGS_*`). |
| `cnt` | `uint8_t` | - | On-wire correction count; may exceed `corrections.size()`. |
| `corrections` | `castle::container::vector<nav_slas_correction, MaxCorrections>` | - | Stored correction blocks. |

### `nav_slas_correction` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `gnss_id` | `uint8_t` | - | GNSS identifier. |
| `sv_id` | `uint8_t` | - | Satellite identifier. |
| `prc` | `int16_t` | cm | Pseudorange correction (block offset 6). |

### Functions

| Function | Description |
| --- | --- |
| `nav_slas::gms_available()`, `qzss_sv_available()`, `test_mode()` | `serviceFlags` bits. |
| `nav_slas::gms_longitude_deg()` / `gms_latitude_deg()` | Station position in degrees. |
| `nav_slas_correction::prc_m()` | Pseudorange correction in metres. |
| `static bool nav_slas::matches(const message_view&)` | Class/ID check. |
| `static bool nav_slas::decode(const message_view&, nav_slas&)` | Validates the exact length, decodes the header and up to `MaxCorrections` blocks. |
| `void nav_slas::encode(payload_writer&) const` | Writes the header (`cnt` from `corrections.size()`) and every stored block; use with `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_slas.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    nav_slas<> slas;
    if (nav_slas<>::decode(raw, slas) && slas.gms_available() && !slas.test_mode())
    {
        for (const nav_slas_correction& correction : slas.corrections)
        {
            // correction.gnss_id, correction.sv_id, correction.prc_m() ...
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/nav_slas.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Capacity is `MaxCorrections` (default 16). Blocks beyond capacity are skipped; `cnt` keeps the on-wire count while `corrections.size()` is the stored count.
- No heap allocation; the vector is embedded in the value.
- `decode()` returns `false` and resets the output unless the class/ID match and the payload length is exactly `20 + cnt·8`.
- `encode()` takes `cnt` from `corrections.size()`; `reserved0`, `reserved1` and `reserved2` are written as zero.
- Thread-safety: none; plain value type, not thread-safe.
