# UBX NAV-PVT

## Overview

Defines the UBX NAV-PVT navigation solution model with convenience accessors for common scaled position/height/DOP fields.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_pvt` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_pvt::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_pvt::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_PVT`). |
| `nav_pvt::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`92U`). |
| `nav_pvt::VALID_DATE` | `static constexpr uint8_t` protocol/configuration constant (`0x01U`). |
| `nav_pvt::VALID_TIME` | `static constexpr uint8_t` protocol/configuration constant (`0x02U`). |
| `nav_pvt::VALID_FULLY` | `static constexpr uint8_t` protocol/configuration constant (`0x04U`). |
| `nav_pvt::VALID_MAG_DEC` | `static constexpr uint8_t` protocol/configuration constant (`0x08U`). |
| `nav_pvt::FLAGS_GNSS_FIX_OK` | `static constexpr uint8_t` protocol/configuration constant (`0x01U`). |
| `nav_pvt::FLAGS_DIFF_SOLN` | `static constexpr uint8_t` protocol/configuration constant (`0x02U`). |
| `nav_pvt::FLAGS_PSM_STATE_MASK` | `static constexpr uint8_t` protocol/configuration constant (`0x1CU`). |
| `nav_pvt::FLAGS_HEAD_VEH_VALID` | `static constexpr uint8_t` protocol/configuration constant (`0x20U`). |
| `nav_pvt::FLAGS_CARR_SOLN_MASK` | `static constexpr uint8_t` protocol/configuration constant (`0xC0U`). |
| `nav_pvt_fix_type` | Strongly typed protocol enumeration used by the decoded model. |
| `nav_pvt::fix_ok()` | Returns true when the corresponding GNSS-fix-valid flag is set. |
| `nav_pvt::longitude_deg()` | Returns longitude converted from the raw NAV-PVT scale to degrees. |
| `nav_pvt::latitude_deg()` | Returns latitude converted from the raw NAV-PVT scale to degrees. |
| `nav_pvt::height_m()` | Returns ellipsoidal height converted from the raw millimetre scale to metres. |
| `nav_pvt::pdop_value()` | Returns P-DOP converted from the raw 0.01 scale to a floating-point value. |
| `nav_pvt::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_pvt::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_pvt` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `year` | `uint16_t` | `0U` | Calendar year. |
| `month` | `uint8_t` | `0U` | Calendar month. |
| `day` | `uint8_t` | `0U` | Calendar day. |
| `hour` | `uint8_t` | `0U` | Decoded `hour` field. |
| `minute` | `uint8_t` | `0U` | Decoded `minute` field. |
| `second` | `uint8_t` | `0U` | Decoded `second` field. |
| `valid` | `uint8_t` | `0U` | True when decoding completed successfully for the sentence. |
| `t_acc` | `uint32_t` | `0U` | Time accuracy estimate. |
| `nano` | `int32_t` | `0` | Nanosecond/fractional-time component. |
| `fix_type` | `nav_pvt_fix_type` | `nav_pvt_fix_type::no_fix` | Navigation fix-type enumeration. |
| `flags` | `uint8_t` | `0U` | Protocol-defined packed flags. |
| `flags2` | `uint8_t` | `0U` | Secondary packed flags. |
| `num_sv` | `uint8_t` | `0U` | Number of satellites used/reported. |
| `lon` | `int32_t` | `0` | Raw longitude navigation value. |
| `lat` | `int32_t` | `0` | Raw latitude navigation value. |
| `height` | `int32_t` | `0` | Raw ellipsoidal height value. |
| `h_msl` | `int32_t` | `0` | Raw mean-sea-level height value. |
| `h_acc` | `uint32_t` | `0U` | Horizontal accuracy estimate. |
| `v_acc` | `uint32_t` | `0U` | Vertical accuracy estimate. |
| `vel_n` | `int32_t` | `0` | North velocity component. |
| `vel_e` | `int32_t` | `0` | East velocity component. |
| `vel_d` | `int32_t` | `0` | Down velocity component. |
| `g_speed` | `int32_t` | `0` | Ground-speed value. |
| `head_mot` | `int32_t` | `0` | Motion heading. |
| `s_acc` | `uint32_t` | `0U` | Speed accuracy estimate. |
| `head_acc` | `uint32_t` | `0U` | Heading accuracy estimate. |
| `p_dop` | `uint16_t` | `0U` | Position dilution of precision. |
| `flags3` | `uint8_t` | `0U` | Tertiary packed flags. |
| `head_veh` | `int32_t` | `0` | Vehicle heading. |
| `mag_dec` | `int16_t` | `0` | Magnetic declination. |
| `mag_acc` | `uint16_t` | `0U` | Magnetic-declination accuracy estimate. |

### Enumerations

#### `nav_pvt_fix_type`

| Enumerator | Value |
| --- | --- |
| `no_fix` | `0U` |
| `dead_reck` | `1U` |
| `fix_2d` | `2U` |
| `fix_3d` | `3U` |
| `gnss_dr` | `4U` |
| `time_only` | `5U` |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::nav_pvt;

void handle(const message_view& raw) {
    nav_pvt value{};
    if (nav_pvt::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
