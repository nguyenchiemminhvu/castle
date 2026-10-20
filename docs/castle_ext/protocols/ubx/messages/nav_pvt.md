# UBX NAV-PVT

## Overview

Typed model of UBX-NAV-PVT (class 0x01, ID 0x07), the periodic/polled navigation position, velocity and time solution with accuracy figures. The 92-byte layout is shared with UBX-NAV2-PVT through one template, `basic_nav_pvt<MsgClass, MsgId>`; this header defines the NAV instance `nav_pvt`, and [`nav2_pvt.hpp`](nav2_pvt.md) defines `nav2_pvt`.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md) (`castle::math::pow10`)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `template <uint8_t MsgClass, uint8_t MsgId> basic_nav_pvt` | Value type for the payload; derives from `detail::fixed_message<basic_nav_pvt, MsgClass, MsgId, 92U>`. |
| `nav_pvt` | `basic_nav_pvt<UBX_CLASS_NAV, UBX_ID_NAV_PVT>`. |
| `nav_pvt_fix_type` | `enum class : uint8_t` GNSS fix type: `no_fix` 0, `dead_reck` 1, `fix_2d` 2, `fix_3d` 3, `gnss_dr` 4, `time_only` 5. |
| `nav_pvt_psm_state` | `enum class : uint8_t` power save mode state: `not_active` 0, `enabled` 1, `acquisition` 2, `tracking` 3, `power_optimized_tracking` 4, `inactive` 5. |
| `nav_pvt_carr_soln` | `enum class : uint8_t` carrier phase solution: `none` 0, `floating` 1, `fixed` 2. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_pvt::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_pvt::msg_id` | `uint8_t` | `UBX_ID_NAV_PVT` (0x07) | UBX message ID. |
| `nav_pvt::payload_length` | `castle::size_type` | `92U` | Exact payload length. |
| `VALID_DATE` | `uint8_t` | `0x01U` | `valid.validDate`. |
| `VALID_TIME` | `uint8_t` | `0x02U` | `valid.validTime`. |
| `VALID_FULLY` | `uint8_t` | `0x04U` | `valid.fullyResolved`. |
| `VALID_MAG_DEC` | `uint8_t` | `0x08U` | `valid.validMag`. |
| `FLAGS_GNSS_FIX_OK` | `uint8_t` | `0x01U` | `flags.gnssFixOK`. |
| `FLAGS_DIFF_SOLN` | `uint8_t` | `0x02U` | `flags.diffSoln`. |
| `FLAGS_PSM_STATE_MASK` / `FLAGS_PSM_STATE_SHIFT` | `uint8_t` | `0x1CU` / `2U` | `flags.psmState` (bits 4..2). |
| `FLAGS_HEAD_VEH_VALID` | `uint8_t` | `0x20U` | `flags.headVehValid`. |
| `FLAGS_CARR_SOLN_MASK` / `FLAGS_CARR_SOLN_SHIFT` | `uint8_t` | `0xC0U` / `6U` | `flags.carrSoln` (bits 7..6). |
| `FLAGS2_CONFIRMED_AVAI` | `uint8_t` | `0x20U` | `flags2.confirmedAvai`. |
| `FLAGS2_CONFIRMED_DATE` | `uint8_t` | `0x40U` | `flags2.confirmedDate`. |
| `FLAGS2_CONFIRMED_TIME` | `uint8_t` | `0x80U` | `flags2.confirmedTime`. |
| `FLAGS3_INVALID_LLH` | `uint8_t` | `0x01U` | `flags3.invalidLlh`: lon, lat, height and hMSL are invalid. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `year` | `uint16_t` | y | Year (UTC). |
| 6 | `month` | `uint8_t` | month | Month, 1..12 (UTC). |
| 7 | `day` | `uint8_t` | d | Day of month, 1..31 (UTC). |
| 8 | `hour` | `uint8_t` | h | Hour of day, 0..23 (UTC). |
| 9 | `minute` | `uint8_t` | min | Minute of hour, 0..59 (UTC), spec `min`. |
| 10 | `second` | `uint8_t` | s | Seconds of minute, 0..60 (UTC), spec `sec`. |
| 11 | `valid` | `uint8_t` | - | Validity flags (`VALID_*`). |
| 12 | `t_acc` | `uint32_t` | ns | Time accuracy estimate (UTC). |
| 16 | `nano` | `int32_t` | ns | Fraction of second, -1e9..1e9 (UTC). |
| 20 | `fix_type` | `nav_pvt_fix_type` | - | GNSS fix type. |
| 21 | `flags` | `uint8_t` | - | Fix status flags (`FLAGS_*`). |
| 22 | `flags2` | `uint8_t` | - | Additional flags (`FLAGS2_*`). |
| 23 | `num_sv` | `uint8_t` | - | Satellites used in the navigation solution. |
| 24 | `lon` | `int32_t` | 1e-7 deg | Longitude. |
| 28 | `lat` | `int32_t` | 1e-7 deg | Latitude. |
| 32 | `height` | `int32_t` | mm | Height above ellipsoid. |
| 36 | `h_msl` | `int32_t` | mm | Height above mean sea level. |
| 40 | `h_acc` | `uint32_t` | mm | Horizontal accuracy estimate. |
| 44 | `v_acc` | `uint32_t` | mm | Vertical accuracy estimate. |
| 48 | `vel_n` | `int32_t` | mm/s | NED north velocity. |
| 52 | `vel_e` | `int32_t` | mm/s | NED east velocity. |
| 56 | `vel_d` | `int32_t` | mm/s | NED down velocity. |
| 60 | `g_speed` | `int32_t` | mm/s | Ground speed (2-D). |
| 64 | `head_mot` | `int32_t` | 1e-5 deg | Heading of motion (2-D). |
| 68 | `s_acc` | `uint32_t` | mm/s | Speed accuracy estimate. |
| 72 | `head_acc` | `uint32_t` | 1e-5 deg | Heading accuracy estimate (motion and vehicle). |
| 76 | `p_dop` | `uint16_t` | 0.01 | Position DOP. |
| 78 | `flags3` | `uint8_t` | - | Additional flags (`FLAGS3_*`). |
| 79 | - | `U1[5]` | - | Reserved; skipped on decode, zero on encode. |
| 84 | `head_veh` | `int32_t` | 1e-5 deg | Heading of vehicle (2-D); equals heading of motion unless `head_veh_valid()`. |
| 88 | `mag_dec` | `int16_t` | 1e-2 deg | Magnetic declination (ADR 4.10 and later). |
| 90 | `mag_acc` | `uint16_t` | 1e-2 deg | Magnetic declination accuracy (ADR 4.10 and later). |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True when `raw` carries this class/ID. |
| `static bool decode(const message_view& raw, nav_pvt& out)` | `bool` | Decodes an exactly 92-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()`. |
| `valid_date()`, `valid_time()`, `fully_resolved()`, `valid_mag()` | `bool` | `valid` bit tests. |
| `fix_ok()`, `diff_soln()`, `head_veh_valid()` | `bool` | `flags` bit tests. |
| `psm_state()` | `nav_pvt_psm_state` | `flags` bits 4..2, shifted. |
| `carr_soln()` | `nav_pvt_carr_soln` | `flags` bits 7..6, shifted. |
| `confirmed_avai()`, `confirmed_date()`, `confirmed_time()` | `bool` | `flags2` bit tests. |
| `invalid_llh()` | `bool` | `flags3` bit 0. |
| `longitude_deg()`, `latitude_deg()` | `double` | `lon` / `lat` × 1e-7. |
| `height_m()`, `h_msl_m()`, `h_acc_m()`, `v_acc_m()` | `double` | mm × 1e-3. |
| `vel_n_m_s()`, `vel_e_m_s()`, `vel_d_m_s()`, `g_speed_m_s()`, `s_acc_m_s()` | `double` | mm/s × 1e-3. |
| `head_mot_deg()`, `head_acc_deg()`, `head_veh_deg()` | `double` | 1e-5 deg × 1e-5. |
| `pdop_value()` | `double` | `p_dop` × 0.01. |
| `mag_dec_deg()`, `mag_acc_deg()` | `double` | 1e-2 deg × 1e-2. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_pvt;
using castle::protocols::ubx::messages::nav_pvt_carr_soln;

void on_message(const message_view& raw)
{
    nav_pvt pvt{};
    if (nav_pvt::decode(raw, pvt) && pvt.fix_ok() && !pvt.invalid_llh())
    {
        const double lat = pvt.latitude_deg();
        const double speed = pvt.g_speed_m_s();
        const bool rtk_fixed = pvt.carr_soln() == nav_pvt_carr_soln::fixed;
        static_cast<void>(lat); static_cast<void>(speed); static_cast<void>(rtk_fixed);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_pvt.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_pvt.cpp).

## Constraints & Notes

- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 92 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the helpers convert to `double` on demand, so decoding performs no floating-point math.
- `fix_type` stores the raw byte; values above 5 are not remapped.
- `flags3` bits 7..1 are not defined by the ZED-F9R interface description and have no accessor; the raw byte stays available.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
