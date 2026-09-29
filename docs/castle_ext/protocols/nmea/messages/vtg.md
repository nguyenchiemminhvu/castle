# NMEA VTG

## Overview

Represents an NMEA VTG course-and-speed sentence. It decodes true/magnetic course, speed in knots/km/h, magnetic-course validity, and the optional mode character.

## Header

`#include "castle_ext/protocols/nmea/messages/vtg.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)

## Public API

| API | Description |
| --- | --- |
| `vtg` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `vtg::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `vtg::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `vtg` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `course_true` | `double` | `0.0` | True course/track value. |
| `course_mag` | `double` | `0.0` | Magnetic course/track value when supplied. |
| `speed_knots` | `double` | `0.0` | Speed over ground in knots. |
| `speed_kmh` | `double` | `0.0` | Speed over ground in km/h. |
| `course_mag_valid` | `bool` | `false` | Indicates whether the magnetic-course field was present and parsed. |
| `mode` | `char` | `'\0'` | NMEA mode character when provided. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/vtg.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::vtg;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
vtg value{};

if (vtg::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
