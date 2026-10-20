# UBX NAV-COV

## Overview

Typed model of UBX-NAV-COV (class 0x01, ID 0x36), the periodic/polled position and velocity covariance matrices in the local North/East/Down frame. The matrices are symmetric, so only the six upper-triangular entries of each are transmitted.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_cov.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_cov` | Value type for the payload; derives from `detail::fixed_message<nav_cov, UBX_CLASS_NAV, UBX_ID_NAV_COV, 64U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_cov::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_cov::msg_id` | `uint8_t` | `UBX_ID_NAV_COV` | UBX message ID. |
| `nav_cov::payload_length` | `castle::size_type` | `64U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 4 | `version` | `uint8_t` | - | Message version (0x00 for this layout); stored, not validated. |
| 5 | `pos_cov_valid` | `uint8_t` | - | Position covariance matrix validity flag. |
| 6 | `vel_cov_valid` | `uint8_t` | - | Velocity covariance matrix validity flag. |
| 7 | - | `U1[9]` | - | Reserved; skipped on decode, written as zero. |
| 16 | `pos_cov_nn` | `float` | m^2 | Position covariance p_NN (R4). |
| 20 | `pos_cov_ne` | `float` | m^2 | Position covariance p_NE (R4). |
| 24 | `pos_cov_nd` | `float` | m^2 | Position covariance p_ND (R4). |
| 28 | `pos_cov_ee` | `float` | m^2 | Position covariance p_EE (R4). |
| 32 | `pos_cov_ed` | `float` | m^2 | Position covariance p_ED (R4). |
| 36 | `pos_cov_dd` | `float` | m^2 | Position covariance p_DD (R4). |
| 40 | `vel_cov_nn` | `float` | m^2/s^2 | Velocity covariance v_NN (R4). |
| 44 | `vel_cov_ne` | `float` | m^2/s^2 | Velocity covariance v_NE (R4). |
| 48 | `vel_cov_nd` | `float` | m^2/s^2 | Velocity covariance v_ND (R4). |
| 52 | `vel_cov_ee` | `float` | m^2/s^2 | Velocity covariance v_EE (R4). |
| 56 | `vel_cov_ed` | `float` | m^2/s^2 | Velocity covariance v_ED (R4). |
| 60 | `vel_cov_dd` | `float` | m^2/s^2 | Velocity covariance v_DD (R4). |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_cov::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_COV`. |
| `static bool nav_cov::decode(const message_view& raw, nav_cov& out)` | `bool` | Decodes an exactly 64-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_cov.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_cov;

void on_message(const message_view& raw)
{
    nav_cov value{};
    if (nav_cov::matches(raw) && nav_cov::decode(raw, value))
    {
        if (value.pos_cov_valid != 0U)
        {
            const float north_variance = value.pos_cov_nn; // [m^2]
            static_cast<void>(north_variance);
        }
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_cov.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_cov.cpp).

## Constraints & Notes

- R4 fields are stored as `float` and transferred bit-exactly (`castle::bit_cast`), so NaN/Inf payloads round-trip unchanged.
- Validity flags are exposed as raw `uint8_t` as in the spec (non-zero = valid).
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 64 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
