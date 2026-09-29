# UBX TIM-TP

## Overview

Defines the UBX TIM-TP time-pulse model with time-of-week, quantization error, week, flags, and reference information.

## Header

`#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `tim_tp` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `tim_tp::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_TIM`). |
| `tim_tp::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_TIM_TP`). |
| `tim_tp::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`16U`). |
| `tim_tp::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `tim_tp::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `tim_tp` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `tow_ms` | `uint32_t` | `0U` | Time-of-week in milliseconds. |
| `tow_sub_ms` | `uint32_t` | `0U` | Sub-millisecond time-of-week component. |
| `q_err` | `int32_t` | `0` | Time-pulse quantization error. |
| `week` | `uint16_t` | `0U` | GPS week number. |
| `flags` | `uint8_t` | `0U` | Protocol-defined packed flags. |
| `ref_info` | `uint8_t` | `0U` | Time-pulse reference-information field. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::tim_tp;

void handle(const message_view& raw) {
    tim_tp value{};
    if (tim_tp::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
