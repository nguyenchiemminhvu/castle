# UBX NAV-POSECEF

## Overview

Typed model of UBX-NAV-POSECEF (class 0x01, ID 0x01), the periodic/polled position solution in Earth-centred, Earth-fixed coordinates with integer-centimetre resolution.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_posecef.hpp"`

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
| `nav_posecef` | Value type for the payload; derives from `detail::fixed_message<nav_posecef, UBX_CLASS_NAV, UBX_ID_NAV_POSECEF, 20U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_posecef::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_posecef::msg_id` | `uint8_t` | `UBX_ID_NAV_POSECEF` | UBX message ID. |
| `nav_posecef::payload_length` | `castle::size_type` | `20U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `ecef_x` | `int32_t` | cm | ECEF X coordinate. |
| 8 | `ecef_y` | `int32_t` | cm | ECEF Y coordinate. |
| 12 | `ecef_z` | `int32_t` | cm | ECEF Z coordinate. |
| 16 | `p_acc` | `uint32_t` | cm | Position accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_posecef::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_POSECEF`. |
| `static bool nav_posecef::decode(const message_view& raw, nav_posecef& out)` | `bool` | Decodes an exactly 20-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |
| `double ecef_x_m() const` | `double` | `ecef_x` × 1e-2. |
| `double ecef_y_m() const` | `double` | `ecef_y` × 1e-2. |
| `double ecef_z_m() const` | `double` | `ecef_z` × 1e-2. |
| `double p_acc_m() const` | `double` | `p_acc` × 1e-2. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_posecef.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_posecef;

void on_message(const message_view& raw)
{
    nav_posecef value{};
    if (nav_posecef::matches(raw) && nav_posecef::decode(raw, value))
    {
        const double x = value.ecef_x_m();
        static_cast<void>(x);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_posecef.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_posecef.cpp).

## Constraints & Notes

- For 0.1 mm resolution use UBX-NAV-HPPOSECEF (`nav_hpposecef.hpp`).
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 20 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
