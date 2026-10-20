# UBX NAV2-PVT

## Overview

Typed model of UBX-NAV2-PVT (class 0x29, ID 0x07), the secondary-output navigation position, velocity and time solution. Its payload is identical to UBX-NAV-PVT, so `nav2_pvt` is an alias of the shared template `basic_nav_pvt` with the NAV2 class/ID; every field, mask, enum and helper is documented in [`nav_pvt.md`](nav_pvt.md).

## Header

`#include "castle_ext/protocols/ubx/messages/nav2_pvt.hpp"`

## Dependencies

- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/messages/nav_pvt.hpp`](nav_pvt.md) (`basic_nav_pvt`, `nav_pvt_fix_type`, `nav_pvt_psm_state`, `nav_pvt_carr_soln`)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav2_pvt` | `basic_nav_pvt<UBX_CLASS_NAV2, UBX_ID_NAV2_PVT>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav2_pvt::msg_class` | `uint8_t` | `UBX_CLASS_NAV2` (0x29) | UBX class. |
| `nav2_pvt::msg_id` | `uint8_t` | `UBX_ID_NAV2_PVT` (0x07) | UBX message ID. |
| `nav2_pvt::payload_length` | `castle::size_type` | `92U` | Exact payload length. |
| `VALID_*`, `FLAGS_*`, `FLAGS2_*`, `FLAGS3_*` | `uint8_t` | see [`nav_pvt.md`](nav_pvt.md) | Same bit masks as NAV-PVT. |

### Fields

Identical to [`nav_pvt`](nav_pvt.md#fields): `i_tow`, `year`, `month`, `day`, `hour`, `minute`, `second`, `valid`, `t_acc`, `nano`, `fix_type` (`nav_pvt_fix_type`), `flags`, `flags2`, `num_sv`, `lon`, `lat`, `height`, `h_msl`, `h_acc`, `v_acc`, `vel_n`, `vel_e`, `vel_d`, `g_speed`, `head_mot`, `s_acc`, `head_acc`, `p_dop`, `flags3`, `head_veh`, `mag_dec`, `mag_acc`.

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True only for class 0x29 / ID 0x07 (a NAV-PVT frame does not match). |
| `static bool decode(const message_view& raw, nav2_pvt& out)` | `bool` | Decodes an exactly 92-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload; used by `encode_message()`. |
| Bit accessors and scaled helpers | - | Same as [`nav_pvt`](nav_pvt.md#functions) (`fix_ok()`, `carr_soln()`, `psm_state()`, `invalid_llh()`, `latitude_deg()`, `g_speed_m_s()`, ...). |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav2_pvt.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav2_pvt;

void on_message(const message_view& raw)
{
    nav2_pvt pvt{};
    if (nav2_pvt::decode(raw, pvt) && pvt.fix_ok())
    {
        const double heading = pvt.head_veh_deg();
        static_cast<void>(heading);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav2_pvt.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav2_pvt.cpp).

## Constraints & Notes

- `nav2_pvt` and `nav_pvt` are distinct types (different template arguments) sharing one layout definition, so the two cannot drift.
- The former `nav2_pvt_fix_type` enum and the `min` / `sec` field names were removed; use `nav_pvt_fix_type`, `minute` and `second`.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); `decode()` rejects any payload that is not exactly 92 bytes.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
