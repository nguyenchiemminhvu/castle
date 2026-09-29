# UBX INF

## Overview

Defines a bounded UBX INF text message model. The message ID determines the INF subtype and the text is copied into caller-owned fixed storage with no heap allocation.

## Header

`#include "castle_ext/protocols/ubx/messages/inf.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)

## Public API

| API | Description |
| --- | --- |
| `inf<MaxText=256U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `inf_subtype` | Strongly typed protocol enumeration used by the decoded model. |
| `inf::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `inf::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |
| `inf::text()` | Returns a non-owning `string_view` over the copied INF text. |

### `inf` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `msg_class` | `uint8_t` | `UBX_CLASS_INF` | UBX message class byte. |
| `message_id` | `uint8_t` | `0U` | UBX INF message identifier and subtype selector. |
| `subtype` | `inf_subtype` | `inf_subtype::error` | Decoded INF subtype derived from message_id. |
| `text_storage` | `castle::container::array<char,MaxText>` | `—` | Fixed-capacity character storage for the copied INF text. |
| `text_length` | `castle::size_type` | `0U` | Number of characters copied into text_storage. |

### Enumerations

#### `inf_subtype`

| Enumerator | Value |
| --- | --- |
| `error` | `0x00U` |
| `warning` | `0x01U` |
| `notice` | `0x02U` |
| `test` | `0x03U` |
| `debug` | `0x04U` |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/inf.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::inf;

void handle(const message_view& raw) {
    inf<128U> value{};
    if (inf::decode(raw, value)) {
        // The fixed-capacity arrays contain at most the configured bound.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Template capacity parameters bound retained records/text; excess protocol records are skipped or ignored after validation rather than dynamically allocated.
- INF text is copied up to `MaxText` bytes and is exposed through `text()` as a non-owning view over that fixed storage.
