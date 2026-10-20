# UBX NAV-HPPOSECEF

## Overview

Typed model of UBX-NAV-HPPOSECEF (class 0x01, ID 0x13), the periodic/polled high-precision ECEF position. Each coordinate is a centimetre part plus a 0.1 mm high-precision part; per the spec, precise coordinate [cm] = ecefX + ecefXHp × 1e-2.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_hpposecef.hpp"`

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
| `nav_hpposecef` | Value type for the payload; derives from `detail::fixed_message<nav_hpposecef, UBX_CLASS_NAV, UBX_ID_NAV_HPPOSECEF, 28U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_hpposecef::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_hpposecef::msg_id` | `uint8_t` | `UBX_ID_NAV_HPPOSECEF` | UBX message ID. |
| `nav_hpposecef::payload_length` | `castle::size_type` | `28U` | Exact payload length. |
| `nav_hpposecef::FLAGS_INVALID_ECEF` | `uint8_t` | `0x01U` | `flags` bit 0 `invalidEcef`: coordinates and HP parts invalid. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `version` | `uint8_t` | - | Message version (0x00 for this layout); stored, not validated. |
| 1 | - | `U1[3]` | - | Reserved; skipped on decode, written as zero. |
| 4 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 8 | `ecef_x` | `int32_t` | cm | ECEF X coordinate. |
| 12 | `ecef_y` | `int32_t` | cm | ECEF Y coordinate. |
| 16 | `ecef_z` | `int32_t` | cm | ECEF Z coordinate. |
| 20 | `ecef_x_hp` | `int8_t` | 0.1 mm | High-precision part of X, -99..+99. |
| 21 | `ecef_y_hp` | `int8_t` | 0.1 mm | High-precision part of Y, -99..+99. |
| 22 | `ecef_z_hp` | `int8_t` | 0.1 mm | High-precision part of Z, -99..+99. |
| 23 | `flags` | `uint8_t` | bitfield | Additional flags, see `FLAGS_*`. |
| 24 | `p_acc` | `uint32_t` | 0.1 mm | Position accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_hpposecef::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_HPPOSECEF`. |
| `static bool nav_hpposecef::decode(const message_view& raw, nav_hpposecef& out)` | `bool` | Decodes an exactly 28-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |
| `bool invalid_ecef() const` | `bool` | `flags` bit 0 set. |
| `double ecef_x_m() const` | `double` | `ecef_x` × 1e-2 + `ecef_x_hp` × 1e-4 [m]. |
| `double ecef_y_m() const` | `double` | `ecef_y` × 1e-2 + `ecef_y_hp` × 1e-4 [m]. |
| `double ecef_z_m() const` | `double` | `ecef_z` × 1e-2 + `ecef_z_hp` × 1e-4 [m]. |
| `double p_acc_m() const` | `double` | `p_acc` × 1e-4 [m]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_hpposecef.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_hpposecef;

void on_message(const message_view& raw)
{
    nav_hpposecef value{};
    if (nav_hpposecef::matches(raw) && nav_hpposecef::decode(raw, value))
    {
        if (!value.invalid_ecef())
        {
            const double x = value.ecef_x_m(); // cm + 0.1 mm parts combined
            static_cast<void>(x);
        }
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_hpposecef.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_hpposecef.cpp).

## Constraints & Notes

- The `-99..+99` range of the HP parts is documented by the spec but not enforced; out-of-range values decode and combine arithmetically.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 28 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
