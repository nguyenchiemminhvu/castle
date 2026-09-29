# NMEA DTM

## Overview

Represents an NMEA DTM datum-reference sentence. It decodes the talker identifier, local datum offsets, and optional reference datum without owning the source sentence buffer.

## Header

`#include "castle_ext/protocols/nmea/messages/dtm.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)

## Public API

| API | Description |
| --- | --- |
| `dtm` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `dtm::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `dtm::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `dtm` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `datum_code` | `castle::container::string_view` | `—` | Primary datum code from the DTM sentence. |
| `datum_sub` | `castle::container::string_view` | `—` | Datum subdivision/extension code from DTM. |
| `ref_datum` | `castle::container::string_view` | `—` | Reference datum identifier from DTM. |
| `lat_offset` | `double` | `0.0` | Decoded latitude/datum offset value from DTM. |
| `lon_offset` | `double` | `0.0` | Decoded longitude/datum offset value from DTM. |
| `alt_offset` | `double` | `0.0` | Decoded altitude/datum offset value from DTM. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/dtm.hpp"

using castle::protocols::nmea::message_view;
using castle::protocols::nmea::messages::dtm;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
dtm value{};

if (dtm::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
