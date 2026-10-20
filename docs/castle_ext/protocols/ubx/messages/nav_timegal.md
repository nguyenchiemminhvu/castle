# UBX NAV-TIMEGAL

## Overview

Model of the periodic/polled UBX-NAV-TIMEGAL output (0x01 0x25): the precise Galileo time of the most recent navigation solution with validity flags and an accuracy estimate.

UBX-NAV-TIMEBDS, UBX-NAV-TIMEGAL and UBX-NAV-TIMEQZSS are byte-identical 20-byte payloads that differ only in field and flag names. Their layout is written once in [`detail::nav_gnss_time`](detail/nav_gnss_time.md); `nav_timegal` derives from it and adds const accessors carrying the spec names (`gal_tow()`, `f_gal_tow()`, `gal_wno()`). The storage keeps the generic names `tow` / `f_tow` / `week`, so code generic over the three messages works on any of them, while spec-named code reads the accessors. No field is stored twice, and the accessors compile to a plain member load.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timegal.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/nav_gnss_time.hpp`](detail/nav_gnss_time.md) (layout, generic fields and validity helpers), built on [`detail/fixed_message.hpp`](detail/fixed_message.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md) (`castle::math::pow10`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_timegal` | `struct nav_timegal : detail::nav_gnss_time<nav_timegal, UBX_ID_NAV_TIMEGAL>`, the decoded 20-byte payload. |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_timegal::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_timegal::msg_id` | `uint8_t` | `UBX_ID_NAV_TIMEGAL` |
| `nav_timegal::payload_length` | `castle::size_type` | `20U` |
| `nav_timegal::VALID_GAL_TOW` | `uint8_t` | `0x01U`, `galTowValid` |
| `nav_timegal::VALID_GAL_WNO` | `uint8_t` | `0x02U`, `galWnoValid` |
| `nav_timegal::VALID_TOW` | `uint8_t` | `0x01U` (inherited generic name) |
| `nav_timegal::VALID_WEEK` | `uint8_t` | `0x02U` (inherited generic name) |
| `nav_timegal::VALID_LEAP_S` | `uint8_t` | `0x04U`, `leapSValid` |

### Fields (inherited from `nav_gnss_time`)

| Field | Offset | Type | Unit | Spec name |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch |
| `tow` | 4 | `uint32_t` | s | `galTow`: Galileo time of week, rounded |
| `f_tow` | 8 | `int32_t` | ns | `fGalTow`: fraction of `tow`, +/-500000000 |
| `week` | 12 | `int16_t` | - | `galWno`: Galileo week number |
| `leap_s` | 14 | `int8_t` | s | `leapS`: Galileo leap seconds (Galileo - UTC) |
| `valid` | 15 | `uint8_t` | - | `valid`: validity flags |
| `t_acc` | 16 | `uint32_t` | ns | `tAcc`: time accuracy estimate |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `gal_tow()` | `uint32_t` | `galTow`, same value as `tow` [s]. |
| `f_gal_tow()` | `int32_t` | `fGalTow`, same value as `f_tow` [ns]. |
| `gal_wno()` | `int16_t` | `galWno`, same value as `week`. |
| `gal_tow_valid()` | `bool` | `galTowValid` (bit 0). |
| `gal_wno_valid()` | `bool` | `galWnoValid` (bit 1). |
| `tow_valid()` / `week_valid()` / `leap_s_valid()` | `bool` | Generic validity accessors (inherited). |
| `time_of_week_s()` | `double` | Precise time of week `tow + f_tow * 1e-9` [s]. |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_timegal& out)` | `bool` | Requires class/ID match and payload length exactly 20; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 20-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timegal.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timegal;

void handle(const message_view& raw)
{
    nav_timegal time{};
    if (nav_timegal::decode(raw, time) && time.gal_tow_valid() && time.gal_wno_valid())
    {
        const double gst_tow_s = time.time_of_week_s(); // galTow + fGalTow * 1e-9
        const int16_t gst_week = time.gal_wno();
        (void)gst_tow_s;
        (void)gst_week;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_timegal.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_timegal.cpp).

## Constraints & Notes

- Fixed 20-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
- UBX-NAV-TIMEBDS and UBX-NAV-TIMEQZSS decode the same bytes but have different message IDs, so `matches()` never confuses them.
