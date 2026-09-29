# UBX CFG-VALGET

## Overview

Defines a bounded UBX CFG-VALGET decoded model. Key/value entries are stored in a fixed Castle array and the key width is resolved from the UBX configuration key ID.

## Header

`#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/ubx_config.hpp`](../ubx_config.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `cfg_valget_entry` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `cfg_valget<MaxEntries=32U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `cfg_valget::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_CFG`). |
| `cfg_valget::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_CFG_VALGET`). |
| `cfg_valget::header_length` | `static constexpr castle::size_type` protocol/configuration constant (`4U`). |
| `cfg_valget::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `cfg_valget::value_size()` | Returns the value width associated with a CFG key ID. |
| `cfg_valget::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `cfg_valget_entry` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `key_id` | `uint32_t` | `0U` | UBX configuration key identifier. |
| `value` | `uint64_t` | `0U` | Decoded configuration value widened to uint64_t. |

### `cfg_valget` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `layer` | `uint8_t` | `0U` | CFG value layer selector. |
| `position` | `uint16_t` | `0U` | CFG-VALGET position/index field. |
| `entries` | `castle::container::array<cfg_valget_entry,MaxEntries>` | `—` | Fixed-capacity Castle container holding decoded records/values. |
| `entry_count` | `castle::size_type` | `0U` | Number of configuration entries stored in the bounded array. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::cfg_valget;

void handle(const message_view& raw) {
    cfg_valget<16U> value{};
    if (cfg_valget::decode(raw, value)) {
        // The fixed-capacity arrays contain at most the configured bound.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must be at least the fixed header length; variable records are then validated against their encoded block count.
- Template capacity parameters bound retained records/text; excess protocol records are skipped or ignored after validation rather than dynamically allocated.
