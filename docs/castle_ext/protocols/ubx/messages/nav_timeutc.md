# UBX NAV-TIMEUTC

## Overview

Model of the periodic/polled UBX-NAV-TIMEUTC output (0x01 0x21): the UTC date and time of the most recent navigation solution, with validity flags and the UTC standard the receiver is steered to.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md) (supplies `matches()`, `decode()`, `encode()`; payloads are read/written through [`payload_reader`](../payload_reader.md) / [`payload_writer`](../payload_writer.md))

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_timeutc` | `struct nav_timeutc : detail::fixed_message<nav_timeutc, UBX_CLASS_NAV, UBX_ID_NAV_TIMEUTC, 20U>`. |
| `nav_timeutc_utc_standard` | `enum class : uint8_t`, `utcStandard` (bits 7..4 of `valid`): `not_available` (0), `crl` (1), `nist` (2), `usno` (3), `bipm` (4), `european_laboratories` (5), `former_soviet_union` (6), `ntsc` (7), `unknown` (15). Also reused by UBX-TIM-TP. |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_timeutc::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_timeutc::msg_id` | `uint8_t` | `UBX_ID_NAV_TIMEUTC` |
| `nav_timeutc::payload_length` | `castle::size_type` | `20U` |
| `nav_timeutc::VALID_TOW` | `uint8_t` | `0x01U`, `validTOW` |
| `nav_timeutc::VALID_WKN` | `uint8_t` | `0x02U`, `validWKN` |
| `nav_timeutc::VALID_UTC` | `uint8_t` | `0x04U`, `validUTC` |
| `nav_timeutc::VALID_UTC_STANDARD_MASK` | `uint8_t` | `0xF0U`, `utcStandard` |
| `nav_timeutc::VALID_UTC_STANDARD_SHIFT` | `uint8_t` | `4U` |

### Fields

| Field | Offset | Type | Unit | Description |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch. |
| `t_acc` | 4 | `uint32_t` | ns | `tAcc`: UTC time accuracy estimate. |
| `nano` | 8 | `int32_t` | ns | `nano`: fraction of second, -1e9..1e9. |
| `year` | 12 | `uint16_t` | y | `year`: 1999..2099. |
| `month` | 14 | `uint8_t` | month | `month`: 1..12. |
| `day` | 15 | `uint8_t` | d | `day`: 1..31. |
| `hour` | 16 | `uint8_t` | h | `hour`: 0..23. |
| `min` | 17 | `uint8_t` | min | `min`: 0..59. |
| `sec` | 18 | `uint8_t` | s | `sec`: 0..60 (60 during a leap second). |
| `valid` | 19 | `uint8_t` | - | `valid`: validity flags (bits 2..0) and `utcStandard` (bits 7..4). |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `valid_tow()` | `bool` | `validTOW`. |
| `valid_wkn()` | `bool` | `validWKN`. |
| `valid_utc()` | `bool` | `validUTC`: the UTC date/time fields are valid. |
| `utc_standard()` | `nav_timeutc_utc_standard` | Bits 7..4 of `valid`, shifted down. |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_timeutc& out)` | `bool` | Requires class/ID match and payload length exactly 20; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 20-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_timeutc;

void handle(const message_view& raw)
{
    nav_timeutc time{};
    if (nav_timeutc::decode(raw, time) && time.valid_utc())
    {
        const bool steered_to_usno = time.utc_standard() == castle::protocols::ubx::messages::nav_timeutc_utc_standard::usno;
        const uint16_t year = time.year;
        (void)steered_to_usno;
        (void)year;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_timeutc.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_timeutc.cpp).

## Constraints & Notes

- Fixed 20-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- Raw wire integers are stored unscaled; helpers convert on demand. Bits the spec does not define in `valid` are kept verbatim.
- Not thread-safe; synchronise externally if shared.
- `utc_standard()` returns reserved nibble values (8..14) unchanged as the enum's underlying value; `utcStandard` is not supported for protocol versions below 15.00.
- During a leap second there may be 61 (or 59) seconds in a minute; `sec` may read 60.
