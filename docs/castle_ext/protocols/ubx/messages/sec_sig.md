# UBX SEC-SIG

## Overview

Defines the UBX SEC-SIG jamming/spoofing-status model, including derived enable/state values from the packed status flags.

## Header

`#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `sec_sig` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `sec_sig::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_SEC`). |
| `sec_sig::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_SEC_SIG`). |
| `sec_sig::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`12U`). |
| `sec_sig::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `sec_sig::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `sec_sig` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `jam_flags` | `uint8_t` | `0U` | Jamming status bitfield. |
| `jam_det_enabled` | `uint8_t` | `0U` | Jamming detection enabled flag derived from jam_flags. |
| `jamming_state` | `uint8_t` | `0U` | Jamming state derived from jam_flags. |
| `spf_flags` | `uint8_t` | `0U` | Spoofing status bitfield. |
| `spf_det_enabled` | `uint8_t` | `0U` | Spoofing detection enabled flag derived from spf_flags. |
| `spoofing_state` | `uint8_t` | `0U` | Spoofing state derived from spf_flags. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::sec_sig;

void handle(const message_view& raw) {
    sec_sig value{};
    if (sec_sig::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must be at least the protocol `payload_length` minimum; any additional payload bytes remain available to the decoder.
- `jam_det_enabled`, `jamming_state`, `spf_det_enabled`, and `spoofing_state` are derived from the packed status bytes during decoding.
