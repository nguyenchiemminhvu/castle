# UBX MON-VER

## Overview

Defines a bounded UBX MON-VER model containing fixed-size software/hardware version strings and a fixed-capacity extension list.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"`

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
| `mon_ver<MaxExtensions=10U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `mon_ver::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_MON`). |
| `mon_ver::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_MON_VER`). |
| `mon_ver::sw_version_len` | `static constexpr castle::size_type` protocol/configuration constant (`30U`). |
| `mon_ver::hw_version_len` | `static constexpr castle::size_type` protocol/configuration constant (`10U`). |
| `mon_ver::extension_len` | `static constexpr castle::size_type` protocol/configuration constant (`30U`). |
| `mon_ver::min_payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`40U`). |
| `mon_ver::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `mon_ver::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `mon_ver` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sw_version` | `char` | `—` | Software-version field copied from the payload. |
| `hw_version` | `char` | `—` | Hardware-version field copied from the payload. |
| `extensions` | `castle::container::array<castle::container::array<char,extension_len>,MaxExtensions>` | `—` | Fixed-capacity MON-VER extension strings. |
| `extension_count` | `castle::size_type` | `0U` | Number of MON-VER extension fields stored. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::mon_ver;

void handle(const message_view& raw) {
    mon_ver<4U> value{};
    if (mon_ver::decode(raw, value)) {
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
- MON-VER software/hardware fields are copied into fixed-size buffers of 30 and 10 bytes; extension fields are stored in 30-byte slots.
