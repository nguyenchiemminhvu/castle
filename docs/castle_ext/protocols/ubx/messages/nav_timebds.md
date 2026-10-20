# UBX NAV-TIMEBDS

## Overview

Model of the periodic/polled UBX-NAV-TIMEBDS output (0x01 0x24): the precise BeiDou (BDS) time of the most recent navigation solution with validity flags and an accuracy estimate.

UBX-NAV-TIMEBDS, UBX-NAV-TIMEGAL and UBX-NAV-TIMEQZSS are byte-identical 20-byte payloads that differ only in field and flag names. Their layout is written once in [`detail::nav_gnss_time`](detail/nav_gnss_time.md); `nav_timebds` derives from it and adds const accessors carrying the spec names (`sow()`, `f_sow()`). The storage keeps the generic names `tow` / `f_tow` / `week`, so code generic over the three messages works on any of them, while spec-named code reads the accessors. No field is stored twice, and the accessors compile to a plain member load.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timebds.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/nav_gnss_time.hpp`](detail/nav_gnss_time.md) (layout, generic fields and validity helpers), built on [`detail/fixed_message.hpp`](detail/fixed_message.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md) (`castle::math::pow10`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_timebds` | `struct nav_timebds : detail::nav_gnss_time<nav_timebds, UBX_ID_NAV_TIMEBDS>`, the decoded 20-byte payload. |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_timebds::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_timebds::msg_id` | `uint8_t` | `UBX_ID_NAV_TIMEBDS` |
| `nav_timebds::payload_length` | `castle::size_type` | `20U` |
| `nav_timebds::VALID_SOW` | `uint8_t` | `0x01U`, `sowValid` |
| `nav_timebds::VALID_TOW` | `uint8_t` | `0x01U` (inherited generic name) |
| `nav_timebds::VALID_WEEK` | `uint8_t` | `0x02U` (inherited generic name) |
| `nav_timebds::VALID_LEAP_S` | `uint8_t` | `0x04U`, `leapSValid` |

### Fields (inherited from `nav_gnss_time`)

| Field | Offset | Type | Unit | Spec name |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch |
| `tow` | 4 | `uint32_t` | s | `SOW`: BeiDou (BDS) time of week, rounded |
| `f_tow` | 8 | `int32_t` | ns | `fSOW`: fraction of `tow`, +/-500000000 |
| `week` | 12 | `int16_t` | - | `week`: BeiDou (BDS) week number |
| `leap_s` | 14 | `int8_t` | s | `leapS`: BDS leap seconds (BDS - UTC) |
| `valid` | 15 | `uint8_t` | - | `valid`: validity flags |
| `t_acc` | 16 | `uint32_t` | ns | `tAcc`: time accuracy estimate |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `sow()` | `uint32_t` | `SOW`, same value as `tow` [s]. |
| `f_sow()` | `int32_t` | `fSOW`, same value as `f_tow` [ns]. |
| `sow_valid()` | `bool` | `sowValid` (bit 0). |
| `tow_valid()` / `week_valid()` / `leap_s_valid()` | `bool` | Generic validity accessors (inherited). |
| `time_of_week_s()` | `double` | Precise time of week `tow + f_tow * 1e-9` [s]. |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_timebds& out)` | `bool` | Requires class/ID match and payload length exactly 20; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 20-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timebds.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timebds;

void handle(const message_view& raw)
{
    nav_timebds time{};
    if (nav_timebds::decode(raw, time) && time.sow_valid() && time.week_valid())
    {
        const double bds_tow_s = time.time_of_week_s(); // SOW + fSOW * 1e-9
        const int16_t bds_week = time.week;
        (void)bds_tow_s;
        (void)bds_week;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_timebds.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_timebds.cpp).

## Constraints & Notes

- Fixed 20-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
- UBX-NAV-TIMEGAL and UBX-NAV-TIMEQZSS decode the same bytes but have different message IDs, so `matches()` never confuses them.
