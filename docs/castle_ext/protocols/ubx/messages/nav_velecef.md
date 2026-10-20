# UBX NAV-VELECEF

## Overview

Typed model of UBX-NAV-VELECEF (class 0x01, ID 0x11), the periodic/polled velocity solution in Earth-centred, Earth-fixed coordinates.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_velecef.hpp"`

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
| `nav_velecef` | Value type for the payload; derives from `detail::fixed_message<nav_velecef, UBX_CLASS_NAV, UBX_ID_NAV_VELECEF, 20U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_velecef::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_velecef::msg_id` | `uint8_t` | `UBX_ID_NAV_VELECEF` | UBX message ID. |
| `nav_velecef::payload_length` | `castle::size_type` | `20U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `ecef_vx` | `int32_t` | cm/s | ECEF X velocity. |
| 8 | `ecef_vy` | `int32_t` | cm/s | ECEF Y velocity. |
| 12 | `ecef_vz` | `int32_t` | cm/s | ECEF Z velocity. |
| 16 | `s_acc` | `uint32_t` | cm/s | Speed accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_velecef::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_VELECEF`. |
| `static bool nav_velecef::decode(const message_view& raw, nav_velecef& out)` | `bool` | Decodes an exactly 20-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |
| `double ecef_vx_m_s() const` | `double` | `ecef_vx` × 1e-2 [m/s]. |
| `double ecef_vy_m_s() const` | `double` | `ecef_vy` × 1e-2 [m/s]. |
| `double ecef_vz_m_s() const` | `double` | `ecef_vz` × 1e-2 [m/s]. |
| `double s_acc_m_s() const` | `double` | `s_acc` × 1e-2 [m/s]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_velecef.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_velecef;

void on_message(const message_view& raw)
{
    nav_velecef value{};
    if (nav_velecef::matches(raw) && nav_velecef::decode(raw, value))
    {
        const double vx = value.ecef_vx_m_s();
        static_cast<void>(vx);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_velecef.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_velecef.cpp).

## Constraints & Notes

- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 20 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
