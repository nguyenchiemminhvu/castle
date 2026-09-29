# UBX NAV-STATUS

## Overview

Defines the UBX NAV-STATUS receiver-status model with fix type, status flags, TTFF, and milliseconds since startup.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_status.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `nav_status` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `nav_status::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_NAV`). |
| `nav_status::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_NAV_STATUS`). |
| `nav_status::payload_length` | `static constexpr castle::size_type` protocol/configuration constant (`16U`). |
| `nav_status::FLAGS_GPS_FIX_OK` | `static constexpr uint8_t` protocol/configuration constant (`0x01U`). |
| `nav_status::FLAGS_DIFF_SOLN` | `static constexpr uint8_t` protocol/configuration constant (`0x02U`). |
| `nav_status::FLAGS_WKN_SET` | `static constexpr uint8_t` protocol/configuration constant (`0x04U`). |
| `nav_status::FLAGS_TOW_SET` | `static constexpr uint8_t` protocol/configuration constant (`0x08U`). |
| `nav_status_gps_fix` | Strongly typed protocol enumeration used by the decoded model. |
| `nav_status::fix_ok()` | Returns true when the corresponding GNSS-fix-valid flag is set. |
| `nav_status::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `nav_status::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `nav_status` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `0U` | GNSS time-of-week field. |
| `gps_fix` | `nav_status_gps_fix` | `nav_status_gps_fix::no_fix` | Receiver GPS fix-type enumeration. |
| `flags` | `uint8_t` | `0U` | Protocol-defined packed flags. |
| `fix_stat` | `uint8_t` | `0U` | Fix-status bitfield. |
| `flags2` | `uint8_t` | `0U` | Secondary packed flags. |
| `ttff` | `uint32_t` | `0U` | Time-to-first-fix metric. |
| `msss` | `uint32_t` | `0U` | Milliseconds since startup. |

### Enumerations

#### `nav_status_gps_fix`

| Enumerator | Value |
| --- | --- |
| `no_fix` | `0U` |
| `dead_reck` | `1U` |
| `fix_2d` | `2U` |
| `fix_3d` | `3U` |
| `gnss_dr` | `4U` |
| `time_only` | `5U` |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_status.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_status;

void handle(const message_view& raw) {
    nav_status value{};
    if (nav_status::decode(raw, value)) {
        // `value` contains the decoded, typed fields.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must exactly equal the protocol `payload_length` constant before field extraction.
