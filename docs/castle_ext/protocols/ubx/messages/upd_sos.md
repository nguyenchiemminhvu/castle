# UBX UPD-SOS

## Overview

Defines the UBX UPD-SOS output/status model for supported save/restore commands.

## Header

`#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `upd_sos_output` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `upd_sos_output::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_UPD`). |
| `upd_sos_output::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_UPD_SOS`). |
| `upd_sos_output::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`8U`). |
| `upd_sos_output::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `upd_sos_output::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `upd_sos_output` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `cmd` | `uint8_t` | `0U` | UPD-SOS command/status selector. |
| `response` | `uint8_t` | `0U` | UPD-SOS response/status value. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::upd_sos_output;

void handle(const message_view& raw) {
    upd_sos_output value{};
    if (upd_sos_output::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must be at least the protocol `payload_length` minimum; any additional payload bytes remain available to the decoder.
- Only the supported UPD-SOS command values represented by the header are accepted by `decode()`.
