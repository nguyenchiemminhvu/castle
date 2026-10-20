# UBX NAV-STATUS

## Overview

Typed model of UBX-NAV-STATUS (class 0x01, ID 0x03), the periodic/polled receiver navigation status: GPS fix type, fix/time validity flags, differential and carrier-phase status, map matching, power save and spoofing-detection state, time to first fix and time since startup.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_status.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_status` | Value type for the payload; derives from `detail::fixed_message<nav_status, UBX_CLASS_NAV, UBX_ID_NAV_STATUS, 16U>`. |
| `nav_status_gps_fix` | `enum class : uint8_t` `gpsFix`: `no_fix` 0, `dead_reck` 1, `fix_2d` 2, `fix_3d` 3, `gnss_dr` 4, `time_only` 5 (0x06..0xFF reserved). |
| `nav_status_map_matching` | `enum class : uint8_t` `fixStat.mapMatching`: `none` 0, `valid_not_used` 1, `valid_used` 2, `valid_used_dr` 3. |
| `nav_status_psm_state` | `enum class : uint8_t` `flags2.psmState`: `acquisition` 0 (or PSM disabled), `tracking` 1, `power_optimized_tracking` 2, `inactive` 3. |
| `nav_status_spoof_det_state` | `enum class : uint8_t` `flags2.spoofDetState`: `unknown` 0, `no_spoofing` 1, `spoofing` 2, `multiple` 3. |
| `nav_status_carr_soln` | `enum class : uint8_t` `flags2.carrSoln`: `none` 0, `floating` 1, `fixed` 2. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_status::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_status::msg_id` | `uint8_t` | `UBX_ID_NAV_STATUS` (0x03) | UBX message ID. |
| `nav_status::payload_length` | `castle::size_type` | `16U` | Exact payload length. |
| `FLAGS_GPS_FIX_OK` | `uint8_t` | `0x01U` | `flags.gpsFixOk`. |
| `FLAGS_DIFF_SOLN` | `uint8_t` | `0x02U` | `flags.diffSoln`. |
| `FLAGS_WKN_SET` | `uint8_t` | `0x04U` | `flags.wknSet`. |
| `FLAGS_TOW_SET` | `uint8_t` | `0x08U` | `flags.towSet`. |
| `FIX_STAT_DIFF_CORR` | `uint8_t` | `0x01U` | `fixStat.diffCorr`. |
| `FIX_STAT_CARR_SOLN_VALID` | `uint8_t` | `0x02U` | `fixStat.carrSolnValid`. |
| `FIX_STAT_MAP_MATCHING_MASK` / `_SHIFT` | `uint8_t` | `0xC0U` / `6U` | `fixStat.mapMatching` (bits 7..6). |
| `FLAGS2_PSM_STATE_MASK` / `_SHIFT` | `uint8_t` | `0x03U` / `0U` | `flags2.psmState` (bits 1..0). |
| `FLAGS2_SPOOF_DET_STATE_MASK` / `_SHIFT` | `uint8_t` | `0x18U` / `3U` | `flags2.spoofDetState` (bits 4..3). |
| `FLAGS2_CARR_SOLN_MASK` / `_SHIFT` | `uint8_t` | `0xC0U` / `6U` | `flags2.carrSoln` (bits 7..6). |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `gps_fix` | `nav_status_gps_fix` | - | GPS fix type; does not by itself qualify a fix as valid. |
| 5 | `flags` | `uint8_t` | - | Navigation status flags (`FLAGS_*`). |
| 6 | `fix_stat` | `uint8_t` | - | Fix status information (`FIX_STAT_*`). |
| 7 | `flags2` | `uint8_t` | - | Further navigation output information (`FLAGS2_*`). |
| 8 | `ttff` | `uint32_t` | ms | Time to first fix. |
| 12 | `msss` | `uint32_t` | ms | Milliseconds since startup/reset. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID 0x03. |
| `static bool decode(const message_view& raw, nav_status& out)` | `bool` | Decodes an exactly 16-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload; used by `encode_message()`. |
| `fix_ok()`, `diff_soln()`, `wkn_set()`, `tow_set()` | `bool` | `flags` bit tests. |
| `diff_corr()`, `carr_soln_valid()` | `bool` | `fix_stat` bit tests. |
| `map_matching()` | `nav_status_map_matching` | `fix_stat` bits 7..6, shifted. |
| `psm_state()` | `nav_status_psm_state` | `flags2` bits 1..0. |
| `spoof_det_state()` | `nav_status_spoof_det_state` | `flags2` bits 4..3, shifted. |
| `carr_soln()` | `nav_status_carr_soln` | `flags2` bits 7..6, shifted. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_status.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_status;
using castle::protocols::ubx::messages::nav_status_spoof_det_state;

bool trustworthy_fix(const message_view& raw)
{
    nav_status status{};
    return nav_status::decode(raw, status) && status.fix_ok()
        && (status.spoof_det_state() != nav_status_spoof_det_state::spoofing)
        && (status.spoof_det_state() != nav_status_spoof_det_state::multiple);
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_status.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_status.cpp).

## Constraints & Notes

- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual).
- `decode()` accepts only a payload of exactly 16 bytes; shorter or longer payloads are rejected rather than partially decoded.
- `gps_fix` stores the raw byte; reserved values (0x06..0xFF) are kept as-is.
- `psmState` is not supported before protocol 13.01 and `spoofDetState` not before 18.00; older receivers report 0.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
