# NMEA RMC

## Overview

Represents an NMEA RMC recommended-minimum-navigation sentence. It decodes time, status, position, speed, true course, date, magnetic variation, and the optional mode indicator.

## Header

`#include "castle_ext/protocols/nmea/messages/rmc.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `rmc` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `rmc_mode` | Strongly typed protocol enumeration used by the decoded model. |
| `rmc::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `rmc::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `rmc` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `utc_time` | `double` | `0.0` | UTC time field decoded from the sentence. |
| `latitude` | `double` | `0.0` | Decoded latitude value. |
| `longitude` | `double` | `0.0` | Decoded longitude value. |
| `speed_knots` | `double` | `0.0` | Speed over ground in knots. |
| `course_true` | `double` | `0.0` | True course/track value. |
| `mag_variation` | `double` | `0.0` | Magnetic variation from RMC. |
| `status_active` | `bool` | `false` | True when the sentence status character reports an active solution. |
| `date` | `uint32_t` | `0U` | NMEA date field. |
| `mode` | `rmc_mode` | `rmc_mode::unknown` | NMEA mode character when provided. |

### Enumerations

#### `rmc_mode`

| Enumerator | Value |
| --- | --- |
| `autonomous` | `'A'` |
| `differential` | `'D'` |
| `estimated` | `'E'` |
| `manual` | `'M'` |
| `not_valid` | `'N'` |
| `simulated` | `'S'` |
| `unknown` | `'\0'` |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/rmc.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::rmc;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
rmc value{};

if (rmc::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
