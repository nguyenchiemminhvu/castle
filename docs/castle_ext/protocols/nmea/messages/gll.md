# NMEA GLL

## Overview

Represents an NMEA GLL geographic-position sentence. It decodes latitude/longitude, UTC time, active/inactive status, and optional mode information.

## Header

`#include "castle_ext/protocols/nmea/messages/gll.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)

## Public API

| API | Description |
| --- | --- |
| `gll` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gll::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `gll::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `gll` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `latitude` | `double` | `0.0` | Decoded latitude value. |
| `longitude` | `double` | `0.0` | Decoded longitude value. |
| `utc_time` | `double` | `0.0` | UTC time field decoded from the sentence. |
| `status_active` | `bool` | `false` | True when the sentence status character reports an active solution. |
| `mode` | `char` | `'\0'` | NMEA mode character when provided. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/gll.hpp"

using castle::protocols::nmea::message_view;
using castle::protocols::nmea::messages::gll;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
gll value{};

if (gll::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
