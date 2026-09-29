# UBX ESF-STATUS

## Overview

Defines a bounded UBX ESF-STATUS decoded model with the fusion state and a fixed-capacity list of sensor status records.

## Header

`#include "castle_ext/protocols/ubx/messages/esf_status.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `esf_status_sensor` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `esf_status_sensor::TYPE_MASK` | `static constexpr uint8_t` protocol/configuration constant (`0x1FU`). |
| `esf_status_sensor::USED` | `static constexpr uint8_t` protocol/configuration constant (`0x20U`). |
| `esf_status_sensor::READY` | `static constexpr uint8_t` protocol/configuration constant (`0x40U`). |
| `esf_status_sensor::CALIB_STATUS_MASK` | `static constexpr uint8_t` protocol/configuration constant (`0x03U`). |
| `esf_status_sensor::TIME_STATUS_MASK` | `static constexpr uint8_t` protocol/configuration constant (`0x0CU`). |
| `esf_status<MaxSensors=32U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `esf_status::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_ESF`). |
| `esf_status::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_ESF_STATUS`). |
| `esf_status::min_payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`16U`). |
| `esf_status::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `esf_status::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `esf_status_sensor` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sens_status1` | `uint8_t` | `0U` | Sensor status byte 1. |
| `sens_status2` | `uint8_t` | `0U` | Sensor status byte 2. |
| `freq` | `uint8_t` | `0U` | Sensor update-frequency/status field. |
| `faults` | `uint8_t` | `0U` | Sensor fault bitfield. |

### `esf_status` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `fusion_mode` | `uint8_t` | `0U` | ESF sensor-fusion mode field. |
| `num_sens` | `uint8_t` | `0U` | Number of sensor status records in the payload. |
| `sensors` | `castle::container::array<esf_status_sensor,MaxSensors>` | `—` | Fixed-capacity sensor status records. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/esf_status.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::esf_status;

void handle(const message_view& raw) {
    esf_status<8U> value{};
    if (esf_status::decode(raw, value)) {
        // The fixed-capacity arrays contain at most the configured bound.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must be at least the fixed minimum; optional/variable records are validated from the remaining bytes.
- Template capacity parameters bound retained records/text; excess protocol records are skipped or ignored after validation rather than dynamically allocated.
