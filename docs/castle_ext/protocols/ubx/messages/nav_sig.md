# UBX NAV-SIG

## Overview

Defines a bounded UBX NAV-SIG model containing per-signal tracking/status records.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"`

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
| `nav_sig_info` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_sig<MaxSignals=64U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_sig::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_sig::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_SIG`). |
| `nav_sig::header_length` | `static constexpr castle::size_type` protocol/configuration constant (`8U`). |
| `nav_sig::block_length` | `static constexpr castle::size_type` protocol/configuration constant (`16U`). |
| `nav_sig::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_sig::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_sig_info` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `gnss_id` | `uint8_t` | `0U` | GNSS constellation identifier. |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier. |
| `sig_id` | `uint8_t` | `0U` | Signal identifier. |
| `freq_id` | `uint8_t` | `0U` | Signal frequency identifier. |
| `pr_res` | `int16_t` | `0` | Pseudorange residual. |
| `cno` | `uint8_t` | `0U` | Carrier-to-noise density ratio / signal-strength field. |
| `qual_ind` | `uint8_t` | `0U` | Signal quality indicator. |
| `corr_source` | `uint8_t` | `0U` | Signal correction-source field. |
| `iono_model` | `uint8_t` | `0U` | Ionospheric model field. |
| `sig_flags` | `uint16_t` | `0U` | Signal status bitfield. |

### `nav_sig` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `num_sigs` | `uint8_t` | `0U` | Number of signal records in the payload. |
| `signals` | `castle::container::array<nav_sig_info,MaxSignals>` | `—` | Fixed-capacity signal records. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::nav_sig;

void handle(const message_view& raw) {
    nav_sig<16U> value{};
    if (nav_sig::decode(raw, value)) {
        // The fixed-capacity arrays contain at most the configured bound.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must be at least the fixed header length; variable records are then validated against their encoded block count.
- Wire record counts are accepted only when the payload length exactly matches the encoded header-plus-block calculation; storage is capped by the template parameter.
