# NMEA GNS

## Overview

Represents an NMEA GNS GNSS-fix sentence. It decodes time, position, mode indicator, satellite count, dilution, altitude, geoid separation, and optional differential-reference fields.

## Header

`#include "castle_ext/protocols/nmea/messages/gns.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)

## Public API

| API | Description |
| --- | --- |
| `gns` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gns::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `gns::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `gns` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `mode_indicator` | `castle::container::string_view` | `—` | NMEA GNS mode-indicator field. |
| `utc_time` | `double` | `0.0` | UTC time field decoded from the sentence. |
| `latitude` | `double` | `0.0` | Decoded latitude value. |
| `longitude` | `double` | `0.0` | Decoded longitude value. |
| `hdop` | `double` | `0.0` | Horizontal dilution-of-precision value. |
| `altitude` | `double` | `0.0` | Decoded `altitude` field. |
| `geoid_sep` | `double` | `0.0` | Decoded `geoid sep` field. |
| `diff_age` | `double` | `-1.0` | Age of differential corrections when present. |
| `num_sats` | `int` | `0` | Number of satellites reported by GNS. |
| `diff_ref` | `int` | `-1` | Differential reference station identifier when present. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/gns.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::gns;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
gns value{};

if (gns::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
