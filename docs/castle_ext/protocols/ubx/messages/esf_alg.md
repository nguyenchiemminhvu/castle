# UBX ESF-ALG

## Overview

Model of UBX-ESF-ALG (0x10 0x14), the periodic/polled IMU-mount alignment output of the ZED-F9R: the
alignment angles that rotate the installation frame into the IMU frame, the automatic-alignment status and
the alignment error flags. Fixed 16-byte payload, decoded and encoded from one `layout()` through
`detail::fixed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/esf_alg.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/constants.hpp](../../../../core/constants.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [detail/fixed_message.hpp](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `esf_alg` | UBX-ESF-ALG payload; `fixed_message<esf_alg, UBX_CLASS_ESF, UBX_ID_ESF_ALG, 16U>`. |
| `esf_alg_status : uint8_t` | `flags.status`: `user_defined` (0), `roll_pitch_ongoing` (1), `roll_pitch_yaw_ongoing` (2), `coarse` (3), `fine` (4). |

### Constants

| Name | Value | Meaning |
| --- | --- | --- |
| `msg_class` / `msg_id` / `payload_length` | `0x10` / `0x14` / `16` | Frame identity and exact length. |
| `FLAGS_AUTO_MNT_ALG_ON` | `0x01` | `flags` bit 0: automatic IMU-mount alignment running. |
| `FLAGS_STATUS` | `0x0E` | `flags` bits 3..1: alignment status. |
| `ERROR_TILT_ALG_ERROR` | `0x01` | `error` bit 0: roll/pitch alignment error. |
| `ERROR_YAW_ALG_ERROR` | `0x02` | `error` bit 1: yaw alignment error. |
| `ERROR_ANGLE_ERROR` | `0x04` | `error` bit 2: Euler-angle singularity (gimbal lock). |

### Fields

| Offset | Field | Type | Unit / scale |
| --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms |
| 4 | `version` | `uint8_t` | 0x01 for this version |
| 5 | `flags` | `uint8_t` | `FLAGS_*` bits |
| 6 | `error` | `uint8_t` | `ERROR_*` bits |
| 7 | reserved | 1 byte | written as zero |
| 8 | `yaw` | `uint32_t` | 1e-2 deg, [0, 360] |
| 12 | `pitch` | `int16_t` | 1e-2 deg, [-90, 90] |
| 14 | `roll` | `int16_t` | 1e-2 deg, [-180, 180] |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, esf_alg&)` | Exact-length decode; `out` value-initialised on failure. |
| `void encode(payload_writer&) const` | Used by `encode_message()`. |
| `bool auto_mnt_alg_on() const` | `flags` bit 0. |
| `esf_alg_status status() const` | `flags` bits 3..1. |
| `bool tilt_alg_error() const` / `yaw_alg_error()` / `angle_error()` | `error` bits 0 / 1 / 2. |
| `double yaw_deg() const` / `pitch_deg()` / `roll_deg()` | Raw angle × 1e-2. |

## Usage Example

```cpp
esf_alg alignment;
if (esf_alg::decode(raw, alignment) && (alignment.status() == esf_alg_status::fine))
{
    double yaw = alignment.yaw_deg();
}
// Request one output: encode_poll<esf_alg>(frame, sizeof(frame), written);
```

Full walk-through: `samples/castle_ext/protocols/ubx/messages/esf_alg.cpp`.

## Constraints & Notes

- `decode()` accepts exactly 16 payload bytes; anything else returns `false`.
- `status()` casts the 3-bit field to the enum; values 5..7 are not defined by the spec and come back as
  out-of-enumerator values.
- Scaled helpers are computed on demand; the raw integers are the exact wire values.
- Value type: every member lives in the object; no heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
