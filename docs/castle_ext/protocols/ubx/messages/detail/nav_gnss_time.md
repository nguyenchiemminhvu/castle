# UBX nav_gnss_time

## Overview

`detail::nav_gnss_time` is the single layout behind UBX-NAV-TIMEBDS, UBX-NAV-TIMEGAL and UBX-NAV-TIMEQZSS. The three 20-byte payloads are byte-identical and differ only in field and flag names:

| Offset | Type | Generic field | TIMEBDS | TIMEGAL | TIMEQZSS |
| --- | --- | --- | --- | --- | --- |
| 0 | U4 | `i_tow` | iTOW | iTOW | iTOW |
| 4 | U4 | `tow` | SOW | galTow | qzssTow |
| 8 | I4 | `f_tow` | fSOW | fGalTow | fQzssTow |
| 12 | I2 | `week` | week | galWno | qzssWno |
| 14 | I1 | `leap_s` | leapS | leapS | leapS |
| 15 | X1 | `valid` | valid | valid | valid |
| 16 | U4 | `t_acc` | tAcc | tAcc | tAcc |

### Design choice

The base stores the fields under **generic** names (`tow`, `f_tow`, `week`) and owns the one `layout()`. Each message is a named struct deriving from it ([`nav_timebds`](../nav_timebds.md), [`nav_timegal`](../nav_timegal.md), [`nav_timeqzss`](../nav_timeqzss.md)) that adds only const accessors with the **spec** names (`sow()`, `gal_tow()`, `qzss_wno()`, `gal_tow_valid()`, ...) and spec-named mask aliases (`VALID_GAL_TOW`, ...).

Alternatives that were rejected:

- Three separate structs: triplicates the layout, the exact drift `fixed_message` exists to prevent.
- Spec-named data members plus a per-message `layout()`: still three layouts.
- Reference members aliasing the generic fields: makes the struct larger and non-copyable.

With this design the wire layout exists once, generic code (`tow_valid()`, `time_of_week_s()`) works on all three messages, and each accessor compiles to one member load. Each named struct is still its own type with its own message ID, so `matches()` and the decoder registry tell the three apart.

## Header

`#include "castle_ext/protocols/ubx/messages/detail/nav_gnss_time.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../../core/constants.md) (`castle::math::pow10`)
- [`castle_ext/protocols/ubx/ubx.hpp`](../../ubx.md)
- [`detail/fixed_message.hpp`](fixed_message.md)

## Public API

`template <typename Derived, uint8_t MsgId> struct nav_gnss_time : fixed_message<Derived, UBX_CLASS_NAV, MsgId, 20U>`

| Member | Type | Description |
| --- | --- | --- |
| `msg_class`, `msg_id`, `payload_length` | constants | `UBX_CLASS_NAV`, `MsgId`, `20U` (from `fixed_message`). |
| `VALID_TOW` | `uint8_t` | `0x01U`: `tow`/`f_tow` valid (sowValid / galTowValid / qzssTowValid). |
| `VALID_WEEK` | `uint8_t` | `0x02U`: `week` valid (weekValid / galWnoValid / qzssWnoValid). |
| `VALID_LEAP_S` | `uint8_t` | `0x04U`: `leap_s` valid (leapSValid). |
| `i_tow` | `uint32_t` | GPS time of week of the navigation epoch [ms]. |
| `tow` | `uint32_t` | Constellation time of week, rounded [s]. |
| `f_tow` | `int32_t` | Fractional part of `tow`, +/-500000000 [ns]. |
| `week` | `int16_t` | Constellation week number. |
| `leap_s` | `int8_t` | Constellation leap seconds (system time - UTC) [s]. |
| `valid` | `uint8_t` | Validity flags. |
| `t_acc` | `uint32_t` | Time accuracy estimate [ns]. |
| `bool tow_valid() const` / `week_valid()` / `leap_s_valid()` | `bool` | Test the `VALID_*` bits. |
| `double time_of_week_s() const` | `double` | `tow + f_tow * 1e-9` [s]. |
| `static void layout(Self&, Stream&)` | - | The shared 20-byte layout used by `matches()` / `decode()` / `encode()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/detail/nav_gnss_time.hpp"

namespace ubx = castle::protocols::ubx;

struct nav_timegal : ubx::messages::detail::nav_gnss_time<nav_timegal, ubx::UBX_ID_NAV_TIMEGAL>
{
    int16_t gal_wno() const noexcept { return week; } // spec name over the generic field
};
```

Real messages built on this base: [`nav_timebds`](../nav_timebds.md), [`nav_timegal`](../nav_timegal.md), [`nav_timeqzss`](../nav_timeqzss.md). The test `test/castle_ext/protocols/ubx/messages/detail/nav_gnss_time.cpp` checks the layout with a bare derived probe.

Full walk-through of all three messages over the shared layout: [`samples/castle_ext/protocols/ubx/messages/detail/nav_gnss_time.cpp`](../../../../../../samples/castle_ext/protocols/ubx/messages/detail/nav_gnss_time.cpp).

## Constraints & Notes

- Header-only, no runtime overhead: CRTP, no virtual functions, no heap, no exceptions, no RTTI.
- Exact 20-byte length is enforced by `fixed_message::decode()`; on failure `out` is value-initialised.
- Not for UBX-NAV-TIMEGLO (different layout: `U2 Nt`, `U1 N4`) or UBX-NAV-TIMEGPS (16 bytes; see `basic_nav_timegps`).
- Not thread-safe; plain value types.
