# UBX ESF-INS

## Overview

Defines the UBX ESF-INS inertial-sensor decoded model, including validity flags, time of week, angular-rate measurements, and acceleration measurements.

## Header

`#include "castle_ext/protocols/ubx/messages/esf_ins.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `esf_ins` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `esf_ins::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_ESF`). |
| `esf_ins::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_ESF_INS`). |
| `esf_ins::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`36U`). |
| `esf_ins::BF0_X_ANG_RATE_VALID` | `static constexpr uint32_t` protocol/configuration constant (`1U`). |
| `esf_ins::BF0_Y_ANG_RATE_VALID` | `static constexpr uint32_t` protocol/configuration constant (`2U`). |
| `esf_ins::BF0_Z_ANG_RATE_VALID` | `static constexpr uint32_t` protocol/configuration constant (`4U`). |
| `esf_ins::BF0_X_ACCEL_VALID` | `static constexpr uint32_t` protocol/configuration constant (`8U`). |
| `esf_ins::BF0_Y_ACCEL_VALID` | `static constexpr uint32_t` protocol/configuration constant (`16U`). |
| `esf_ins::BF0_Z_ACCEL_VALID` | `static constexpr uint32_t` protocol/configuration constant (`32U`). |
| `esf_ins::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `esf_ins::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `esf_ins` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `bitfield0` | `uint32_t` | `0U` | Packed ESF-INS validity/status flags. |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `x_ang_rate` | `int32_t` | `0` | X-axis angular-rate measurement. |
| `y_ang_rate` | `int32_t` | `0` | Y-axis angular-rate measurement. |
| `z_ang_rate` | `int32_t` | `0` | Z-axis angular-rate measurement. |
| `x_accel` | `int32_t` | `0` | X-axis acceleration measurement. |
| `y_accel` | `int32_t` | `0` | Y-axis acceleration measurement. |
| `z_accel` | `int32_t` | `0` | Z-axis acceleration measurement. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/esf_ins.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::esf_ins;

void handle(const message_view& raw) {
    esf_ins value{};
    if (esf_ins::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
