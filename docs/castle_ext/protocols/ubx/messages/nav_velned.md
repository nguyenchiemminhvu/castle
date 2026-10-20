# UBX NAV-VELNED

## Overview

Typed model of UBX-NAV-VELNED (class 0x01, ID 0x12), the periodic/polled velocity solution in the local North/East/Down frame, with 3-D speed, 2-D ground speed and 2-D heading of motion.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_velned.hpp"`

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
| `nav_velned` | Value type for the payload; derives from `detail::fixed_message<nav_velned, UBX_CLASS_NAV, UBX_ID_NAV_VELNED, 36U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_velned::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_velned::msg_id` | `uint8_t` | `UBX_ID_NAV_VELNED` | UBX message ID. |
| `nav_velned::payload_length` | `castle::size_type` | `36U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `vel_n` | `int32_t` | cm/s | North velocity. |
| 8 | `vel_e` | `int32_t` | cm/s | East velocity. |
| 12 | `vel_d` | `int32_t` | cm/s | Down velocity. |
| 16 | `speed` | `uint32_t` | cm/s | 3-D speed. |
| 20 | `g_speed` | `uint32_t` | cm/s | 2-D ground speed. |
| 24 | `heading` | `int32_t` | 1e-5 deg | 2-D heading of motion. |
| 28 | `s_acc` | `uint32_t` | cm/s | Speed accuracy estimate. |
| 32 | `c_acc` | `uint32_t` | 1e-5 deg | Course/heading accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_velned::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_VELNED`. |
| `static bool nav_velned::decode(const message_view& raw, nav_velned& out)` | `bool` | Decodes an exactly 36-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |
| `double vel_n_m_s() const` | `double` | `vel_n` × 1e-2. |
| `double vel_e_m_s() const` | `double` | `vel_e` × 1e-2. |
| `double vel_d_m_s() const` | `double` | `vel_d` × 1e-2. |
| `double speed_m_s() const` | `double` | `speed` × 1e-2. |
| `double g_speed_m_s() const` | `double` | `g_speed` × 1e-2. |
| `double heading_deg() const` | `double` | `heading` × 1e-5. |
| `double s_acc_m_s() const` | `double` | `s_acc` × 1e-2. |
| `double c_acc_deg() const` | `double` | `c_acc` × 1e-5. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_velned.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_velned;

void on_message(const message_view& raw)
{
    nav_velned value{};
    if (nav_velned::matches(raw) && nav_velned::decode(raw, value))
    {
        const double course = value.heading_deg();
        const double ground_speed = value.g_speed_m_s();
        static_cast<void>(course); static_cast<void>(ground_speed);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_velned.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_velned.cpp).

## Constraints & Notes

- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 36 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
