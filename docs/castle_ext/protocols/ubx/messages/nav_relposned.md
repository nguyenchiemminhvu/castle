# UBX NAV-RELPOSNED

## Overview

Typed model of UBX-NAV-RELPOSNED (class 0x01, ID 0x3C), the periodic/polled relative position vector from the reference station (or moving base) to the rover in the NED frame of the reference station, with high-precision parts, accuracies, heading and RTK solution flags. This is the version 0x01, 64-byte layout.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_relposned.hpp"`

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
| `nav_relposned` | Value type for the payload; derives from `detail::fixed_message<nav_relposned, UBX_CLASS_NAV, UBX_ID_NAV_RELPOSNED, 64U>`. |
| `nav_relposned_carr_soln` | `enum class : uint8_t` for `flags.carrSoln`: `none` = 0 (no carrier phase solution), `floating` = 1 (float ambiguities), `fixed` = 2 (fixed ambiguities). |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_relposned::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_relposned::msg_id` | `uint8_t` | `UBX_ID_NAV_RELPOSNED` | UBX message ID. |
| `nav_relposned::payload_length` | `castle::size_type` | `64U` | Exact payload length. |
| `nav_relposned::FLAGS_GNSS_FIX_OK` | `uint32_t` | `0x001U` | bit 0 `gnssFixOK`: valid fix within DOP/accuracy masks. |
| `nav_relposned::FLAGS_DIFF_SOLN` | `uint32_t` | `0x002U` | bit 1 `diffSoln`: differential corrections applied. |
| `nav_relposned::FLAGS_REL_POS_VALID` | `uint32_t` | `0x004U` | bit 2 `relPosValid`: components/accuracies (and moving-base baseline) valid. |
| `nav_relposned::FLAGS_CARR_SOLN_MASK` | `uint32_t` | `0x018U` | bits 4..3 `carrSoln`. |
| `nav_relposned::FLAGS_CARR_SOLN_SHIFT` | `uint32_t` | `3U` | Shift for `carrSoln`. |
| `nav_relposned::FLAGS_IS_MOVING` | `uint32_t` | `0x020U` | bit 5 `isMoving`: moving-base mode. |
| `nav_relposned::FLAGS_REF_POS_MISS` | `uint32_t` | `0x040U` | bit 6 `refPosMiss`: extrapolated reference position used. |
| `nav_relposned::FLAGS_REF_OBS_MISS` | `uint32_t` | `0x080U` | bit 7 `refObsMiss`: extrapolated reference observations used. |
| `nav_relposned::FLAGS_REL_POS_HEADING_VALID` | `uint32_t` | `0x100U` | bit 8 `relPosHeadingValid`. |
| `nav_relposned::FLAGS_REL_POS_NORMALIZED` | `uint32_t` | `0x200U` | bit 9 `relPosNormalized`: components (incl. HP parts) normalised. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `version` | `uint8_t` | - | Message version (0x01 for this layout); stored, not validated. |
| 1 | - | `U1[1]` | - | Reserved; skipped on decode, written as zero. |
| 2 | `ref_station_id` | `uint16_t` | - | Reference station ID, 0..4095. |
| 4 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 8 | `rel_pos_n` | `int32_t` | cm | North component of the relative position vector. |
| 12 | `rel_pos_e` | `int32_t` | cm | East component. |
| 16 | `rel_pos_d` | `int32_t` | cm | Down component. |
| 20 | `rel_pos_length` | `int32_t` | cm | Length of the relative position vector. |
| 24 | `rel_pos_heading` | `int32_t` | 1e-5 deg | Heading of the relative position vector. |
| 28 | - | `U1[4]` | - | Reserved; skipped on decode, written as zero. |
| 32 | `rel_pos_hp_n` | `int8_t` | 0.1 mm | High-precision North part, -99..+99. |
| 33 | `rel_pos_hp_e` | `int8_t` | 0.1 mm | High-precision East part, -99..+99. |
| 34 | `rel_pos_hp_d` | `int8_t` | 0.1 mm | High-precision Down part, -99..+99. |
| 35 | `rel_pos_hp_length` | `int8_t` | 0.1 mm | High-precision length part (spec `relPosHPLength`), -99..+99. |
| 36 | `acc_n` | `uint32_t` | 0.1 mm | North accuracy. |
| 40 | `acc_e` | `uint32_t` | 0.1 mm | East accuracy. |
| 44 | `acc_d` | `uint32_t` | 0.1 mm | Down accuracy. |
| 48 | `acc_length` | `uint32_t` | 0.1 mm | Length accuracy. |
| 52 | `acc_heading` | `uint32_t` | 1e-5 deg | Heading accuracy. |
| 56 | - | `U1[4]` | - | Reserved; skipped on decode, written as zero. |
| 60 | `flags` | `uint32_t` | bitfield | Solution flags, see `FLAGS_*`. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_relposned::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_RELPOSNED`. |
| `static bool nav_relposned::decode(const message_view& raw, nav_relposned& out)` | `bool` | Decodes an exactly 64-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |
| `bool gnss_fix_ok() const` | `bool` | `FLAGS_GNSS_FIX_OK` set. |
| `bool diff_soln() const` | `bool` | `FLAGS_DIFF_SOLN` set. |
| `bool rel_pos_valid() const` | `bool` | `FLAGS_REL_POS_VALID` set. |
| `nav_relposned_carr_soln carr_soln() const` | `nav_relposned_carr_soln` | Bits 4..3 shifted down; the undefined value 3 is passed through as-is. |
| `bool is_moving() const` | `bool` | `FLAGS_IS_MOVING` set. |
| `bool ref_pos_miss() const` | `bool` | `FLAGS_REF_POS_MISS` set. |
| `bool ref_obs_miss() const` | `bool` | `FLAGS_REF_OBS_MISS` set. |
| `bool rel_pos_heading_valid() const` | `bool` | `FLAGS_REL_POS_HEADING_VALID` set. |
| `bool rel_pos_normalized() const` | `bool` | `FLAGS_REL_POS_NORMALIZED` set. |
| `double rel_pos_n_m() const` | `double` | `rel_pos_n` × 1e-2 + `rel_pos_hp_n` × 1e-4 [m]. |
| `double rel_pos_e_m() const` | `double` | `rel_pos_e` × 1e-2 + `rel_pos_hp_e` × 1e-4 [m]. |
| `double rel_pos_d_m() const` | `double` | `rel_pos_d` × 1e-2 + `rel_pos_hp_d` × 1e-4 [m]. |
| `double rel_pos_length_m() const` | `double` | `rel_pos_length` × 1e-2 + `rel_pos_hp_length` × 1e-4 [m]. |
| `double rel_pos_heading_deg() const` | `double` | `rel_pos_heading` × 1e-5. |
| `double acc_n_m() const` | `double` | `acc_n` × 1e-4. |
| `double acc_e_m() const` | `double` | `acc_e` × 1e-4. |
| `double acc_d_m() const` | `double` | `acc_d` × 1e-4. |
| `double acc_length_m() const` | `double` | `acc_length` × 1e-4. |
| `double acc_heading_deg() const` | `double` | `acc_heading` × 1e-5. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_relposned.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_relposned;

void on_message(const message_view& raw)
{
    nav_relposned value{};
    if (nav_relposned::matches(raw) && nav_relposned::decode(raw, value))
    {
        if (value.rel_pos_valid() && value.rel_pos_heading_valid()
            && (value.carr_soln() == castle::protocols::ubx::messages::nav_relposned_carr_soln::fixed))
        {
            const double baseline_m = value.rel_pos_length_m(); // cm + 0.1 mm parts combined
            const double heading = value.rel_pos_heading_deg();
            static_cast<void>(baseline_m); static_cast<void>(heading);
        }
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_relposned.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_relposned.cpp).

## Constraints & Notes

- Only the 64-byte version 0x01 layout is modelled; the 40-byte version 0x00 layout of older firmware fails the length check.
- `refPosMiss`/`refObsMiss` are only set by protocol versions 27.10, 27.11 and 31.11 (per the spec).
- HP part ranges (-99..+99) are documented by the spec but not enforced.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 64 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
