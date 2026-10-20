# UBX ESF-INS

## Overview

Model of UBX-ESF-INS (0x10 0x15), the periodic/polled vehicle-dynamics output: compensated angular rates and
gravity-free accelerations in the vehicle frame, plus per-axis validity flags. Fixed 36-byte payload through
`detail::fixed_message`.

**Change:** the `BF0_*` masks were wrong (bits 0..5). The spec puts the message version in `bitfield0` bits 7..0
and the validity flags in bits 8..13; masks are now `0x100`..`0x2000`, with a new `BF0_VERSION`, `version()`
and per-axis validity accessors.

## Header

`#include "castle_ext/protocols/ubx/messages/esf_ins.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/constants.hpp](../../../../core/constants.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [detail/fixed_message.hpp](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `esf_ins` | UBX-ESF-INS payload; `fixed_message<esf_ins, UBX_CLASS_ESF, UBX_ID_ESF_INS, 36U>`. |

### Constants

| Name | Value | Meaning |
| --- | --- | --- |
| `msg_class` / `msg_id` / `payload_length` | `0x10` / `0x15` / `36` | Frame identity and exact length. |
| `BF0_VERSION` | `0x000000FF` | `bitfield0` bits 7..0: message version. |
| `BF0_X_ANG_RATE_VALID` / `BF0_Y_ANG_RATE_VALID` / `BF0_Z_ANG_RATE_VALID` | `0x100` / `0x200` / `0x400` | Bits 8 / 9 / 10. |
| `BF0_X_ACCEL_VALID` / `BF0_Y_ACCEL_VALID` / `BF0_Z_ACCEL_VALID` | `0x800` / `0x1000` / `0x2000` | Bits 11 / 12 / 13. |

### Fields

| Offset | Field | Type | Unit / scale |
| --- | --- | --- | --- |
| 0 | `bitfield0` | `uint32_t` | version + `BF0_*` bits |
| 4 | reserved | 4 bytes | written as zero |
| 8 | `i_tow` | `uint32_t` | ms |
| 12 / 16 / 20 | `x_ang_rate` / `y_ang_rate` / `z_ang_rate` | `int32_t` | 1e-3 deg/s |
| 24 / 28 / 32 | `x_accel` / `y_accel` / `z_accel` | `int32_t` | 1e-2 m/s^2 |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, esf_ins&)` | Exact-length decode; `out` value-initialised on failure. |
| `void encode(payload_writer&) const` | Used by `encode_message()`. |
| `uint8_t version() const` | `bitfield0` bits 7..0. |
| `bool x_ang_rate_valid() const` … `z_accel_valid() const` | One accessor per validity bit. |
| `double x_ang_rate_deg_s() const` … `z_ang_rate_deg_s()` | Raw × 1e-3. |
| `double x_accel_m_s2() const` … `z_accel_m_s2()` | Raw × 1e-2. |

## Usage Example

```cpp
esf_ins dynamics;
if (esf_ins::decode(raw, dynamics) && dynamics.z_ang_rate_valid())
{
    double yaw_rate = dynamics.z_ang_rate_deg_s();
}
```

Full walk-through: `samples/castle_ext/protocols/ubx/messages/esf_ins.cpp`.

## Constraints & Notes

- `decode()` accepts exactly 36 payload bytes and checks the class/ID; anything else returns `false`.
- Check the matching `*_valid()` accessor before using an axis value.
- Value type: every member lives in the object; no heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
