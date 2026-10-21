# RTCM rtk_observables_family

## Overview

The legacy RTK observables are two families of four consecutive message numbers with the same structure: GPS 1001-1004 and GLONASS 1009-1012. `rtk_observables_family.hpp` derives the compile-time layout of **one** of those numbers from its offset to the family's first number, so [`gps_rtk_observables`](../gps_rtk_observables.md) and [`glonass_rtk_observables`](../glonass_rtk_observables.md) inherit the same constants instead of spelling them twice. As for the MSM models, each message number is its own type; the number is a template argument, not a run-time field.

| Offset | GPS | GLONASS | Content | `IS_EXTENDED` | `HAS_L2` |
| --- | --- | --- | --- | --- | --- |
| 0 | 1001 | 1009 | L1-only | `false` | `false` |
| 1 | 1002 | 1010 | L1-only, extended (ambiguity, CNR) | `true` | `false` |
| 2 | 1003 | 1011 | L1 and L2 | `false` | `true` |
| 3 | 1004 | 1012 | L1 and L2, extended | `true` | `true` |

## Header

`#include "castle_ext/protocols/rtcm/messages/detail/rtk_observables_family.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../../core/compiler.md)

## Public API

`template <uint16_t First, uint16_t Type> struct castle::protocols::rtcm::messages::detail::rtk_observables_family`

`First` is the L1-only number of the family (1001 for GPS, 1009 for GLONASS), `Type` the message number of the model. A `static_assert` requires `First <= Type <= First + 3`; any other `Type` is a compile error.

| Member | Description |
| --- | --- |
| `static constexpr uint16_t FIRST_TYPE` | `First`, the L1-only message number. |
| `static constexpr uint16_t LAST_TYPE` | `First + 3`, the extended L1/L2 message number. |
| `static constexpr uint16_t MESSAGE_TYPE` | `Type`, the message number (DF002) the model writes. |
| `static constexpr bool IS_EXTENDED` | `true` for offsets 1 and 3: the message carries the ambiguity and CNR fields. |
| `static constexpr bool HAS_L2` | `true` for offsets 2 and 3: the message carries the L2 block. |
| `static constexpr bool accepts(uint16_t type)` | `type == MESSAGE_TYPE`. O(1). |

The struct is empty; deriving from it adds no storage to a message model.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/gps_rtk_observables.hpp"

namespace rtcm = castle::protocols::rtcm;
using observables = rtcm::messages::gps_rtk_l1_l2_obs_ext;

static_assert(observables::MESSAGE_TYPE == 1004U, "the alias fixes the message number");
static_assert(observables::IS_EXTENDED && observables::HAS_L2, "1004 is extended L1/L2");
static_assert(!observables::accepts(1003U), "1003 is a different model");
```

See [`samples/castle_ext/protocols/rtcm/messages/detail/rtk_observables_family.cpp`](../../../../../../samples/castle_ext/protocols/rtcm/messages/detail/rtk_observables_family.cpp).

## Constraints & Notes

- Implementation detail of the legacy observables models; use the constants through the models or their aliases (`gps_rtk_l1_obs` ... `glonass_rtk_l1_l2_obs_ext`).
- `accepts()` is `true` for exactly one number; the other three numbers of the family belong to sibling models.
- `IS_EXTENDED` and `HAS_L2` are compile-time constants, so the models drop the fields a message does not carry at compile time and there is no run-time message-number check.
- All members are `constexpr` and usable in `static_assert`.
