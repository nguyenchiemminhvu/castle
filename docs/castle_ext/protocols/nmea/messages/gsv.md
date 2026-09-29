# NMEA GSV

## Overview

Represents an NMEA GSV satellite-in-view sentence. It stores message numbering, satellites-in-view, up to four satellite blocks per sentence, and an optional signal identifier.

## Header

`#include "castle_ext/protocols/nmea/messages/gsv.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `gsv_satellite` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gsv` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gsv::max_satellites` | `static constexpr castle::size_type` protocol/configuration constant (`4U`). |
| `gsv::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `gsv::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `gsv_satellite` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier. |
| `elevation` | `int8_t` | `-1` | Satellite elevation value. |
| `azimuth` | `uint16_t` | `0U` | Satellite azimuth value. |
| `snr` | `int8_t` | `-1` | Satellite signal-to-noise ratio when present. |

### `gsv` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `num_msgs` | `uint8_t` | `0U` | Total GSV message count in the sequence. |
| `msg_num` | `uint8_t` | `0U` | Current GSV message number in the sequence. |
| `sats_in_view` | `uint8_t` | `0U` | Total satellites reported as being in view. |
| `sat_count` | `uint8_t` | `0U` | Number of GSV satellite blocks decoded into the fixed-capacity array. |
| `signal_id` | `uint8_t` | `0U` | Optional GSV signal identifier. |
| `satellites` | `castle::container::array<gsv_satellite,max_satellites>` | `—` | Fixed-capacity Castle container holding decoded records/values. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/gsv.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::gsv;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
gsv value{};

if (gsv::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
- Only the first four satellite blocks are stored per sentence (`max_satellites == 4U`); additional wire blocks are not retained.
