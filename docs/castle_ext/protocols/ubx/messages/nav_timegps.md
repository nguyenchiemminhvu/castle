# UBX NAV-TIMEGPS

## Overview

Model of the periodic/polled UBX-NAV-TIMEGPS output (0x01 0x20): the precise GPS time of the most recent navigation solution with validity flags and an accuracy estimate.

UBX-NAV2-TIMEGPS (0x29 0x20) carries the same 16-byte payload from the secondary navigation output, so the layout lives once in `basic_nav_timegps<MsgClass, MsgId>` and both messages are aliases of it (see [`nav2_timegps`](nav2_timegps.md)), the same pattern `basic_mga_gps_eph<MsgId>` uses for MGA-GPS / MGA-QZSS.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timegps.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md) (supplies `matches()`, `decode()`, `encode()`; payloads are read/written through [`payload_reader`](../payload_reader.md) / [`payload_writer`](../payload_writer.md))
- [`castle/core/constants.hpp`](../../../../core/constants.md) (`castle::math::pow10`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `basic_nav_timegps<MsgClass, MsgId>` | `template <uint8_t MsgClass, uint8_t MsgId> struct basic_nav_timegps : detail::fixed_message<basic_nav_timegps, MsgClass, MsgId, 16U>`, the shared 16-byte GPS time layout. |
| `nav_timegps` | `basic_nav_timegps<UBX_CLASS_NAV, UBX_ID_NAV_TIMEGPS>`. |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_timegps::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_timegps::msg_id` | `uint8_t` | `UBX_ID_NAV_TIMEGPS` |
| `nav_timegps::payload_length` | `castle::size_type` | `16U` |
| `nav_timegps::VALID_TOW` | `uint8_t` | `0x01U`, `towValid` |
| `nav_timegps::VALID_WEEK` | `uint8_t` | `0x02U`, `weekValid` |
| `nav_timegps::VALID_LEAP_S` | `uint8_t` | `0x04U`, `leapSValid` |

### Fields

| Field | Offset | Type | Unit | Description |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch. |
| `f_tow` | 4 | `int32_t` | ns | `fTOW`: fraction of `i_tow`, +/-500000. |
| `week` | 8 | `int16_t` | - | `week`: GPS week number of the navigation epoch. |
| `leap_s` | 10 | `int8_t` | s | `leapS`: GPS leap seconds (GPS - UTC). |
| `valid` | 11 | `uint8_t` | - | `valid`: validity flags. |
| `t_acc` | 12 | `uint32_t` | ns | `tAcc`: time accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `tow_valid()` | `bool` | `towValid`: `i_tow` and `f_tow` are valid. |
| `week_valid()` | `bool` | `weekValid`: `week` is valid. |
| `leap_s_valid()` | `bool` | `leapSValid`: `leap_s` is valid. |
| `time_of_week_s()` | `double` | Precise GPS time of week `i_tow * 1e-3 + f_tow * 1e-9` [s]. |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_timegps& out)` | `bool` | Requires class/ID match and payload length exactly 16; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 16-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timegps.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timegps;

void handle(const message_view& raw)
{
    nav_timegps time{};
    if (nav_timegps::decode(raw, time) && time.tow_valid() && time.week_valid())
    {
        const double gps_tow_s = time.time_of_week_s(); // iTOW * 1e-3 + fTOW * 1e-9
        const int16_t gps_week = time.week;
        (void)gps_tow_s;
        (void)gps_week;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_timegps.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_timegps.cpp).

## Constraints & Notes

- Fixed 16-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
- `nav_timegps` and `nav2_timegps` are distinct types (different class template arguments); `matches()` tells them apart by class.
