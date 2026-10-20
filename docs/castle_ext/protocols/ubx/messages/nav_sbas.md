# UBX NAV-SBAS

## Overview

`nav_sbas<MaxSvs>` models UBX-NAV-SBAS (0x01 0x32), the periodic/polled SBAS status data: a 12-byte header followed by `cnt` 12-byte SV blocks (`12 + cnt·12`). The header carries the GEO in use, SBAS mode and system, the service bitmask and status flags; each `nav_sbas_sv` carries the per-SV monitoring status, services, pseudorange and ionosphere corrections.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_sbas.hpp"`

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
| `nav_sbas_mode : uint8_t` | `disabled = 0`, `enabled_integrity = 1`, `enabled_test_mode = 3`. |
| `nav_sbas_system : int8_t` | `unknown = -1`, `waas = 0`, `egnos = 1`, `msas = 2`, `gagan = 3`, `gps = 16`. |
| `nav_sbas_integrity_used : uint8_t` | `unknown = 0`, `not_available = 1`, `gps_only = 2`. |
| `nav_sbas_sv` | One 12-byte SV block. |
| `nav_sbas<MaxSvs = 16U>` | The message; `MaxSvs <= 255`. |

### Constants

| Constant | Value |
| --- | --- |
| `nav_sbas::msg_class` / `msg_id` | `UBX_CLASS_NAV` / `UBX_ID_NAV_SBAS` |
| `nav_sbas::header_length` | `12U` |
| `nav_sbas::block_length` / `nav_sbas_sv::wire_size` | `12U` |
| `nav_sbas::SERVICE_RANGING` / `CORRECTIONS` / `INTEGRITY` / `TESTMODE` / `BAD` | bits 0 / 1 / 2 / 3 / 4 of `service` |
| `nav_sbas_sv::SV_SERVICE_RANGING` / `CORRECTIONS` / `INTEGRITY` / `TESTMODE` / `BAD` | same bits of `svService` |
| `nav_sbas::STATUS_FLAGS_INTEGRITY_USED` | `0x03` (bits 1..0 of `statusFlags`) |

### `nav_sbas` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| `geo` | `uint8_t` | - | PRN of the GEO whose correction and integrity data is used. |
| `mode` | `nav_sbas_mode` | - | SBAS mode. |
| `sys` | `nav_sbas_system` | - | SBAS system. |
| `service` | `uint8_t` | - | Services available (`SERVICE_*`). |
| `cnt` | `uint8_t` | - | On-wire SV count; may exceed `svs.size()`. |
| `status_flags` | `uint8_t` | - | Status flags (`STATUS_FLAGS_*`). |
| `svs` | `castle::container::vector<nav_sbas_sv, MaxSvs>` | - | Stored SV blocks. |

### `nav_sbas_sv` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `svid` | `uint8_t` | - | SV ID. |
| `flags` | `uint8_t` | - | Flags for this SV (bits not specified). |
| `udre` | `uint8_t` | - | Monitoring status. |
| `sv_sys` | `nav_sbas_system` | - | System (same values as `sys`). |
| `sv_service` | `uint8_t` | - | Services available (`SV_SERVICE_*`). |
| `prc` | `int16_t` | cm | Pseudorange correction. |
| `ic` | `int16_t` | cm | Ionosphere correction. |

### Functions

| Function | Description |
| --- | --- |
| `nav_sbas::ranging()`, `corrections()`, `integrity()`, `test_mode()`, `bad()` | `service` bits. |
| `nav_sbas::integrity_used()` | `nav_sbas_integrity_used` from `statusFlags` bits 1..0. |
| `nav_sbas_sv::ranging()`, `corrections()`, `integrity()`, `test_mode()`, `bad()` | `svService` bits. |
| `nav_sbas_sv::prc_m()` / `ic_m()` | Corrections in metres. |
| `static bool nav_sbas::matches(const message_view&)` | Class/ID check. |
| `static bool nav_sbas::decode(const message_view&, nav_sbas&)` | Validates the exact length, decodes the header and up to `MaxSvs` blocks. |
| `void nav_sbas::encode(payload_writer&) const` | Writes the header (`cnt` from `svs.size()`) and every stored block; use with `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_sbas.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    nav_sbas<> sbas;
    if (nav_sbas<>::decode(raw, sbas) && (sbas.mode != nav_sbas_mode::disabled) && sbas.corrections())
    {
        for (const nav_sbas_sv& sv : sbas.svs)
        {
            // sv.svid, sv.prc_m(), sv.ic_m() ...
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/nav_sbas.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Capacity is `MaxSvs` (default 16). Blocks beyond capacity are skipped; `cnt` keeps the on-wire count while `svs.size()` is the stored count.
- No heap allocation; the vector is embedded in the value.
- `decode()` returns `false` and resets the output unless the class/ID match and the payload length is exactly `12 + cnt·12`.
- `encode()` takes `cnt` from `svs.size()`; `reserved0`, `reserved1` and `reserved2` are written as zero.
- `svSys` is typed `U1` in the spec but documented as "same as SYS" (an `I1` with `-1 = unknown`); it is modelled with the same `int8_t` enum, so wire byte `0xFF` reads as `unknown`.
- Enumerated fields store the raw wire byte; values outside the listed enumerators are preserved, not rejected.
- Thread-safety: none; plain value type, not thread-safe.
