# UBX MON-SPAN

## Overview

Defines the UBX MON-SPAN RF spectrum-monitor model. Each RF block contains a fixed spectrum-bin array plus span/resolution/center/PGA values.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_span.hpp"`

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
| `mon_span_rf_block` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `mon_span<MaxRfBlocks=4U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `mon_span::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_MON`). |
| `mon_span::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_MON_SPAN`). |
| `mon_span::header_length` | `static constexpr castle::size_type` protocol/configuration constant (`4U`). |
| `mon_span::block_length` | `static constexpr castle::size_type` protocol/configuration constant (`272U`). |
| `mon_span::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `mon_span::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `mon_span_rf_block` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `spectrum` | `castle::container::array<uint8_t,MON_SPAN_SPECTRUM_BINS>` | `—` | Fixed-capacity RF spectrum-bin values. |
| `span` | `uint32_t` | `0U` | RF spectrum span field. |
| `res` | `uint32_t` | `0U` | RF spectrum resolution field. |
| `center` | `uint32_t` | `0U` | RF center-frequency field. |
| `pga` | `uint8_t` | `0U` | Programmable-gain-amplifier setting/status. |

### `mon_span` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `num_rf_blocks` | `uint8_t` | `0U` | Number of RF blocks decoded. |
| `rf_blocks` | `castle::container::array<mon_span_rf_block,MaxRfBlocks>` | `—` | Fixed-capacity RF block records. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_span.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::mon_span;

void handle(const message_view& raw) {
    mon_span<2U> value{};
    if (mon_span::decode(raw, value)) {
        // The fixed-capacity arrays contain at most the configured bound.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Template capacity parameters bound retained records/text; excess protocol records are skipped or ignored after validation rather than dynamically allocated.
