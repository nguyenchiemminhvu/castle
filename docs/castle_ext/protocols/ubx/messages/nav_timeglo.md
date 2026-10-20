# UBX NAV-TIMEGLO

## Overview

Model of the periodic/polled UBX-NAV-TIMEGLO output (0x01 0x23): the precise GLONASS time of the most recent navigation solution with validity flags and an accuracy estimate.

GLONASS reports time of day plus a day number inside a four-year interval (`Nt`, `N4`) instead of a week number and leap seconds, so this message does not share the BDS/GAL/QZSS layout in [`detail::nav_gnss_time`](detail/nav_gnss_time.md).

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timeglo.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md) (supplies `matches()`, `decode()`, `encode()`; payloads are read/written through [`payload_reader`](../payload_reader.md) / [`payload_writer`](../payload_writer.md))
- [`castle/core/constants.hpp`](../../../../core/constants.md) (`castle::math::pow10`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_timeglo` | `struct nav_timeglo : detail::fixed_message<nav_timeglo, UBX_CLASS_NAV, UBX_ID_NAV_TIMEGLO, 20U>`. |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_timeglo::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_timeglo::msg_id` | `uint8_t` | `UBX_ID_NAV_TIMEGLO` |
| `nav_timeglo::payload_length` | `castle::size_type` | `20U` |
| `nav_timeglo::VALID_TOD` | `uint8_t` | `0x01U`, `todValid` |
| `nav_timeglo::VALID_DATE` | `uint8_t` | `0x02U`, `dateValid` |

### Fields

| Field | Offset | Type | Unit | Description |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch. |
| `tod` | 4 | `uint32_t` | s | `TOD`: GLONASS time of day, rounded. |
| `f_tod` | 8 | `int32_t` | ns | `fTOD`: fraction of `tod`, +/-500000000. |
| `nt` | 12 | `uint16_t` | days | `Nt`: day 1..1461 within the four-year interval starting 1 Jan of the `n4` year. |
| `n4` | 14 | `uint8_t` | - | `N4`: four-year interval number since 1996 (1 = 1996, 2 = 2000, ...). |
| `valid` | 15 | `uint8_t` | - | `valid`: validity flags. |
| `t_acc` | 16 | `uint32_t` | ns | `tAcc`: time accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `tod_valid()` | `bool` | `todValid`: `tod` and `f_tod` are valid. |
| `date_valid()` | `bool` | `dateValid`: `n4` and `nt` are valid. |
| `time_of_day_s()` | `double` | Precise GLONASS time of day `tod + f_tod * 1e-9` [s]. |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_timeglo& out)` | `bool` | Requires class/ID match and payload length exactly 20; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 20-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timeglo.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timeglo;

void handle(const message_view& raw)
{
    nav_timeglo time{};
    if (nav_timeglo::decode(raw, time) && time.tod_valid() && time.date_valid())
    {
        const double glonass_tod_s = time.time_of_day_s(); // TOD + fTOD * 1e-9
        const uint16_t day_in_interval = time.nt;
        (void)glonass_tod_s;
        (void)day_in_interval;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_timeglo.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_timeglo.cpp).

## Constraints & Notes

- Fixed 20-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
