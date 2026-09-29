# NMEA ZDA

## Overview

Represents an NMEA ZDA UTC-date/time sentence. It decodes UTC time, calendar date, and optional local time-zone hour/minute fields.

## Header

`#include "castle_ext/protocols/nmea/messages/zda.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `zda` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `zda::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `zda::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `zda` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `utc_time` | `double` | `0.0` | UTC time field decoded from the sentence. |
| `day` | `uint8_t` | `0U` | Calendar day. |
| `month` | `uint8_t` | `0U` | Calendar month. |
| `year` | `uint16_t` | `0U` | Calendar year. |
| `tz_hour` | `int8_t` | `0` | Local time-zone hour offset from ZDA. |
| `tz_min` | `uint8_t` | `0U` | Local time-zone minute offset from ZDA. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/zda.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::zda;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
zda value{};

if (zda::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
