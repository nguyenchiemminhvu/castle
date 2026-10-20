# UBX NAV2-TIMEGPS

## Overview

Model of the periodic/polled UBX-NAV2-TIMEGPS output (0x29 0x20): the GPS time solution of the secondary navigation output.

The payload is identical to UBX-NAV-TIMEGPS, so this header is a thin alias of `basic_nav_timegps` from [`nav_timegps.hpp`](nav_timegps.md) bound to `UBX_CLASS_NAV2`; no layout is duplicated.

## Header

`#include "castle_ext/protocols/ubx/messages/nav2_timegps.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/messages/nav_timegps.hpp`](nav_timegps.md) (`basic_nav_timegps`, built on [`detail/fixed_message.hpp`](detail/fixed_message.md))

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav2_timegps` | `basic_nav_timegps<UBX_CLASS_NAV2, UBX_ID_NAV2_TIMEGPS>`; all fields and helpers are documented in [`nav_timegps`](nav_timegps.md). |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav2_timegps::msg_class` | `uint8_t` | `UBX_CLASS_NAV2` |
| `nav2_timegps::msg_id` | `uint8_t` | `UBX_ID_NAV2_TIMEGPS` |
| `nav2_timegps::payload_length` | `castle::size_type` | `16U` |
| `nav2_timegps::VALID_TOW` | `uint8_t` | `0x01U`, `towValid` |
| `nav2_timegps::VALID_WEEK` | `uint8_t` | `0x02U`, `weekValid` |
| `nav2_timegps::VALID_LEAP_S` | `uint8_t` | `0x04U`, `leapSValid` |

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
| `static decode(const message_view& raw, nav2_timegps& out)` | `bool` | Requires class/ID match and payload length exactly 16; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 16-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav2_timegps.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav2_timegps;

void handle(const message_view& raw)
{
    nav2_timegps time{};
    if (nav2_timegps::decode(raw, time) && time.tow_valid())
    {
        const double secondary_tow_s = time.time_of_week_s();
        (void)secondary_tow_s;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav2_timegps.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav2_timegps.cpp).

## Constraints & Notes

- Fixed 16-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
- A NAV-TIMEGPS frame (class 0x01) is not accepted by `nav2_timegps::decode()`, and vice versa.
- The ZED-F9R interface description lists the NAV2 class but no separate NAV2-TIMEGPS table; the layout follows UBX-NAV-TIMEGPS (3.15.23).
