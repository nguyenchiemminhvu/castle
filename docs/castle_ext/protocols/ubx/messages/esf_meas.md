# UBX ESF-MEAS

## Overview

Defines a bounded UBX ESF-MEAS decoded model with variable-length 32-bit measurement words and an optional calibration time tag.

## Header

`#include "castle_ext/protocols/ubx/messages/esf_meas.hpp"`

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
| `esf_meas_datum` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `esf_meas<MaxData=32U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `esf_meas::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_ESF`). |
| `esf_meas::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_ESF_MEAS`). |
| `esf_meas::min_payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`8U`). |
| `esf_meas::FLAGS_TIME_TAG_TYPE_MASK` | `static constexpr uint16_t` protocol/configuration constant (`0x0003U`). |
| `esf_meas::FLAGS_TIME_MARK_SENT_MASK` | `static constexpr uint16_t` protocol/configuration constant (`0x0018U`). |
| `esf_meas::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `esf_meas::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `esf_meas_datum` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `data_type` | `uint8_t` | `0U` | ESF sensor measurement type bits. |
| `data_value` | `int32_t` | `0` | Sign-extended 24-bit ESF measurement value. |

### `esf_meas` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `time_tag` | `uint32_t` | `0U` | ESF measurement time tag. |
| `flags` | `uint16_t` | `0U` | Protocol-defined packed flags. |
| `id` | `uint16_t` | `0U` | ESF measurement stream/device identifier. |
| `num_meas` | `uint8_t` | `0U` | Number of measurement records decoded into the bounded array. |
| `data` | `castle::container::array<esf_meas_datum,MaxData>` | `—` | Fixed-capacity Castle container holding decoded records/values. |
| `has_calib_ttag` | `bool` | `false` | True when the payload contains a calibration time tag. |
| `calib_ttag` | `uint32_t` | `0U` | Optional calibration time tag. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/esf_meas.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::esf_meas;

void handle(const message_view& raw) {
    esf_meas<16U> value{};
    if (esf_meas::decode(raw, value)) {
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
