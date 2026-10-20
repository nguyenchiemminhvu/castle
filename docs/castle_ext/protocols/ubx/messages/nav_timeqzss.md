# UBX NAV-TIMEQZSS

## Overview

Model of the periodic/polled UBX-NAV-TIMEQZSS output (0x01 0x27): the precise QZSS time of the most recent navigation solution with validity flags and an accuracy estimate.

UBX-NAV-TIMEBDS, UBX-NAV-TIMEGAL and UBX-NAV-TIMEQZSS are byte-identical 20-byte payloads that differ only in field and flag names. Their layout is written once in [`detail::nav_gnss_time`](detail/nav_gnss_time.md); `nav_timeqzss` derives from it and adds const accessors carrying the spec names (`qzss_tow()`, `f_qzss_tow()`, `qzss_wno()`). The storage keeps the generic names `tow` / `f_tow` / `week`, so code generic over the three messages works on any of them, while spec-named code reads the accessors. No field is stored twice, and the accessors compile to a plain member load.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timeqzss.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/nav_gnss_time.hpp`](detail/nav_gnss_time.md) (layout, generic fields and validity helpers), built on [`detail/fixed_message.hpp`](detail/fixed_message.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md) (`castle::math::pow10`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_timeqzss` | `struct nav_timeqzss : detail::nav_gnss_time<nav_timeqzss, UBX_ID_NAV_TIMEQZSS>`, the decoded 20-byte payload. |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_timeqzss::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_timeqzss::msg_id` | `uint8_t` | `UBX_ID_NAV_TIMEQZSS` |
| `nav_timeqzss::payload_length` | `castle::size_type` | `20U` |
| `nav_timeqzss::VALID_QZSS_TOW` | `uint8_t` | `0x01U`, `qzssTowValid` |
| `nav_timeqzss::VALID_QZSS_WNO` | `uint8_t` | `0x02U`, `qzssWnoValid` |
| `nav_timeqzss::VALID_TOW` | `uint8_t` | `0x01U` (inherited generic name) |
| `nav_timeqzss::VALID_WEEK` | `uint8_t` | `0x02U` (inherited generic name) |
| `nav_timeqzss::VALID_LEAP_S` | `uint8_t` | `0x04U`, `leapSValid` |

### Fields (inherited from `nav_gnss_time`)

| Field | Offset | Type | Unit | Spec name |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch |
| `tow` | 4 | `uint32_t` | s | `qzssTow`: QZSS time of week, rounded |
| `f_tow` | 8 | `int32_t` | ns | `fQzssTow`: fraction of `tow`, +/-500000000 |
| `week` | 12 | `int16_t` | - | `qzssWno`: QZSS week number |
| `leap_s` | 14 | `int8_t` | s | `leapS`: QZSS leap seconds (QZSS - UTC) |
| `valid` | 15 | `uint8_t` | - | `valid`: validity flags |
| `t_acc` | 16 | `uint32_t` | ns | `tAcc`: time accuracy estimate |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `qzss_tow()` | `uint32_t` | `qzssTow`, same value as `tow` [s]. |
| `f_qzss_tow()` | `int32_t` | `fQzssTow`, same value as `f_tow` [ns]. |
| `qzss_wno()` | `int16_t` | `qzssWno`, same value as `week`. |
| `qzss_tow_valid()` | `bool` | `qzssTowValid` (bit 0). |
| `qzss_wno_valid()` | `bool` | `qzssWnoValid` (bit 1). |
| `tow_valid()` / `week_valid()` / `leap_s_valid()` | `bool` | Generic validity accessors (inherited). |
| `time_of_week_s()` | `double` | Precise time of week `tow + f_tow * 1e-9` [s]. |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_timeqzss& out)` | `bool` | Requires class/ID match and payload length exactly 20; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 20-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timeqzss.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timeqzss;

void handle(const message_view& raw)
{
    nav_timeqzss time{};
    if (nav_timeqzss::decode(raw, time) && time.qzss_tow_valid() && time.qzss_wno_valid())
    {
        const double qzsst_tow_s = time.time_of_week_s(); // qzssTow + fQzssTow * 1e-9
        const int16_t qzsst_week = time.qzss_wno();
        (void)qzsst_tow_s;
        (void)qzsst_week;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_timeqzss.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_timeqzss.cpp).

## Constraints & Notes

- Fixed 20-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
- UBX-NAV-TIMEBDS and UBX-NAV-TIMEGAL decode the same bytes but have different message IDs, so `matches()` never confuses them.
