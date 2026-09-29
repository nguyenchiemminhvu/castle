# UBX SEC-CRC

## Overview

Defines the UBX SEC-CRC security-status model containing the CRC type and CRC-32 value.

## Header

`#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `sec_crc` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `sec_crc::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_SEC`). |
| `sec_crc::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_SEC_CRC`). |
| `sec_crc::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`8U`). |
| `sec_crc::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `sec_crc::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `sec_crc` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `crc_type` | `uint8_t` | `0U` | Security CRC type selector. |
| `reserved` | `uint16_t` | `0U` | Reserved protocol field. |
| `crc32` | `uint32_t` | `0U` | CRC-32 value. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::sec_crc;

void handle(const message_view& raw) {
    sec_crc value{};
    if (sec_crc::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must be at least the protocol `payload_length` minimum; any additional payload bytes remain available to the decoder.
