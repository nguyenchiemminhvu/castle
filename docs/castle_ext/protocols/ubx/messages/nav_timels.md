# UBX NAV-TIMELS

## Overview

Model of the periodic/polled UBX-NAV-TIMELS output (0x01 0x26): the current number of leap seconds, its source, and the upcoming (or most recent) leap second event.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timels.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md) (supplies `matches()`, `decode()`, `encode()`; payloads are read/written through [`payload_reader`](../payload_reader.md) / [`payload_writer`](../payload_writer.md))

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_timels` | `struct nav_timels : detail::fixed_message<nav_timels, UBX_CLASS_NAV, UBX_ID_NAV_TIMELS, 24U>`. |
| `nav_timels_src_of_curr_ls` | `enum class : uint8_t`, `srcOfCurrLs`: `firmware_default` (0), `gps_glonass` (1), `gps` (2), `sbas` (3), `beidou` (4), `galileo` (5), `aided_data` (6), `configured` (7), `unknown` (255). |
| `nav_timels_src_of_ls_change` | `enum class : uint8_t`, `srcOfLsChange`: `no_source` (0), `gps` (2), `sbas` (3), `beidou` (4), `galileo` (5), `glonass` (6). |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_timels::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_timels::msg_id` | `uint8_t` | `UBX_ID_NAV_TIMELS` |
| `nav_timels::payload_length` | `castle::size_type` | `24U` |
| `nav_timels::VALID_CURR_LS` | `uint8_t` | `0x01U`, `validCurrLs` |
| `nav_timels::VALID_TIME_TO_LS_EVENT` | `uint8_t` | `0x02U`, `validTimeToLsEvent` |

### Fields

| Field | Offset | Type | Unit | Description |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch. |
| `version` | 4 | `uint8_t` | - | `version`: message version (0x00 for this revision); stored, not enforced. |
| (reserved) | 5 | 3 bytes | - | `reserved0`: skipped on decode, written as zero. |
| `src_of_curr_ls` | 8 | `nav_timels_src_of_curr_ls` | - | `srcOfCurrLs`: source of `curr_ls`. |
| `curr_ls` | 9 | `int8_t` | s | `currLs`: leap seconds since 6 Jan 1980 (GPS - UTC). |
| `src_of_ls_change` | 10 | `nav_timels_src_of_ls_change` | - | `srcOfLsChange`: source of the future event. |
| `ls_change` | 11 | `int8_t` | s | `lsChange`: +1, -1, or 0 (none scheduled / no information). |
| `time_to_ls_event` | 12 | `int32_t` | s | `timeToLsEvent`: > 0 event ahead, 0 now, < 0 past. |
| `date_of_ls_gps_wn` | 16 | `uint16_t` | - | `dateOfLsGpsWn`: GPS week of the next (or last) event. |
| `date_of_ls_gps_dn` | 18 | `uint16_t` | - | `dateOfLsGpsDn`: GPS day of week of the event (GPS/GAL 1 = Sun..7 = Sat; BDS 0 = Sun..6 = Sat). |
| (reserved) | 20 | 3 bytes | - | `reserved1`: skipped on decode, written as zero. |
| `valid` | 23 | `uint8_t` | - | `valid`: validity flags. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `valid_curr_ls()` | `bool` | `validCurrLs`: `curr_ls` is valid. |
| `valid_time_to_ls_event()` | `bool` | `validTimeToLsEvent`: `time_to_ls_event` and the two `date_of_ls_gps_*` fields are valid. |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_timels& out)` | `bool` | Requires class/ID match and payload length exactly 24; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 24-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timels.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timels;

void handle(const message_view& raw)
{
    nav_timels time{};
    if (nav_timels::decode(raw, time) && time.valid_time_to_ls_event())
    {
        const bool insert_second = (time.ls_change > 0) && (time.time_to_ls_event > 0);
        const bool from_gps = time.src_of_ls_change == castle::protocols::ubx::messages::nav_timels_src_of_ls_change::gps;
        (void)insert_second;
        (void)from_gps;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_timels.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_timels.cpp).

## Constraints & Notes

- Fixed 24-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
- Source bytes are stored in the enums verbatim; values the spec does not list (e.g. `srcOfLsChange` = 1) survive decode and re-encode unchanged.
- The spec splits the names at offsets 16/18 across lines (`dateOfLsGps` + `Wn`/`Dn`); they are `dateOfLsGpsWn` / `dateOfLsGpsDn`.
