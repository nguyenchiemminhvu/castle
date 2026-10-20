# UBX NAV-SAT

## Overview

`nav_sat<MaxSvs>` models UBX-NAV-SAT (0x01 0x35), the periodic/polled satellite information output: an 8-byte header followed by `numSvs` 12-byte satellite blocks (`8 + numSvs·12`). Each block is a `nav_sat_sv` with the raw `flags` bitmask plus masks and accessors for every spec sub-field.

The header also defines `nav_sv_health` and `nav_signal_quality`, which [`nav_sig`](nav_sig.md) and [`nav_orb`](nav_orb.md) reuse.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"`

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
| `nav_sv_health : uint8_t` | `unknown = 0`, `healthy = 1`, `unhealthy = 2`. Shared with NAV-SIG and NAV-ORB. |
| `nav_signal_quality : uint8_t` | `no_signal = 0`, `searching = 1`, `acquired = 2`, `unusable = 3`, `code_locked = 4`, `code_carrier_locked = 5` (wire 6 and 7 mean the same as 5). Shared with NAV-SIG. |
| `nav_sat_orbit_source : uint8_t` | `none = 0`, `ephemeris = 1`, `almanac = 2`, `assistnow_offline = 3`, `assistnow_autonomous = 4` (wire 5..7 = other orbit information). |
| `nav_sat_sv` | One 12-byte satellite block. |
| `nav_sat<MaxSvs = 64U>` | The message; `MaxSvs <= 255`. |

### Constants

| Constant | Value |
| --- | --- |
| `nav_sat::msg_class` / `msg_id` | `UBX_CLASS_NAV` / `UBX_ID_NAV_SAT` |
| `nav_sat::header_length` | `8U` |
| `nav_sat::block_length` / `nav_sat_sv::wire_size` | `12U` |
| `nav_sat_sv::FLAGS_QUALITY_IND` | `0x00000007` (bits 2..0) |
| `nav_sat_sv::FLAGS_SV_USED` | `0x00000008` (bit 3) |
| `nav_sat_sv::FLAGS_HEALTH` | `0x00000030` (bits 5..4) |
| `nav_sat_sv::FLAGS_DIFF_CORR` | `0x00000040` (bit 6) |
| `nav_sat_sv::FLAGS_SMOOTHED` | `0x00000080` (bit 7) |
| `nav_sat_sv::FLAGS_ORBIT_SOURCE` | `0x00000700` (bits 10..8) |
| `nav_sat_sv::FLAGS_EPH_AVAIL` / `ALM_AVAIL` / `ANO_AVAIL` / `AOP_AVAIL` | bits 11 / 12 / 13 / 14 |
| `nav_sat_sv::FLAGS_SBAS_CORR_USED` / `RTCM_` / `SLAS_` / `SPARTN_` | bits 16 / 17 / 18 / 19 |
| `nav_sat_sv::FLAGS_PR_CORR_USED` / `CR_` / `DO_` | bits 20 / 21 / 22 |

### `nav_sat_sv` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `gnss_id` | `uint8_t` | - | GNSS identifier. |
| `sv_id` | `uint8_t` | - | Satellite identifier. |
| `cno` | `uint8_t` | dBHz | Carrier-to-noise ratio. |
| `elev` | `int8_t` | deg | Elevation (+/-90; unknown if out of range). |
| `azim` | `int16_t` | deg | Azimuth (0..360). |
| `pr_res` | `int16_t` | 0.1 m | Pseudorange residual. |
| `flags` | `uint32_t` | - | Bitmask, see `FLAGS_*`. |

### `nav_sat` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| `version` | `uint8_t` | - | Message version (0x01). |
| `num_svs` | `uint8_t` | - | On-wire satellite count; may exceed `svs.size()`. |
| `svs` | `castle::container::vector<nav_sat_sv, MaxSvs>` | - | Stored satellite blocks. |

### Functions

| Function | Description |
| --- | --- |
| `nav_sat_sv::quality_ind()` | `nav_signal_quality` from bits 2..0. |
| `nav_sat_sv::health()` | `nav_sv_health` from bits 5..4. |
| `nav_sat_sv::orbit_source()` | `nav_sat_orbit_source` from bits 10..8. |
| `nav_sat_sv::sv_used()`, `diff_corr()`, `smoothed()`, `eph_avail()`, `alm_avail()`, `ano_avail()`, `aop_avail()`, `sbas_corr_used()`, `rtcm_corr_used()`, `slas_corr_used()`, `spartn_corr_used()`, `pr_corr_used()`, `cr_corr_used()`, `do_corr_used()` | Single-bit flags. |
| `nav_sat_sv::pr_res_m()` | Pseudorange residual in metres. |
| `static bool nav_sat::matches(const message_view&)` | Class/ID check. |
| `static bool nav_sat::decode(const message_view&, nav_sat&)` | Validates the exact length, decodes the header and up to `MaxSvs` blocks. |
| `void nav_sat::encode(payload_writer&) const` | Writes the header (count from `svs.size()`) and every stored block; use with `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    nav_sat<32U> sat;
    if (!nav_sat<32U>::decode(raw, sat))
    {
        return;
    }
    for (const nav_sat_sv& sv : sat.svs)
    {
        if (sv.sv_used() && (sv.health() == nav_sv_health::healthy))
        {
            // sv.cno, sv.elev, sv.pr_res_m() ...
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/nav_sat.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Capacity is `MaxSvs` (default 64). Blocks beyond capacity are skipped; `num_svs` keeps the on-wire count while `svs.size()` is the stored count.
- No heap allocation; the vector is embedded in the value.
- `decode()` returns `false` and resets the output unless the class/ID match and the payload length is exactly `8 + numSvs·12`.
- `encode()` takes `numSvs` from `svs.size()`; the `num_svs` member is ignored when encoding. Reserved bytes are written as zero.
- Thread-safety: none; plain value type, not thread-safe.
