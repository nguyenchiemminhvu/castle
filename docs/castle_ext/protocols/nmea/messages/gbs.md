# NMEA GBS

## Overview

Represents an NMEA GBS GNSS satellite-fault-detection sentence. It exposes UTC time, estimated position/altitude error, failed satellite information, and optional probability/bias terms.

## Header

`#include "castle_ext/protocols/nmea/messages/gbs.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `gbs` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gbs::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `gbs::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `gbs` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `utc_time` | `double` | `0.0` | UTC time field decoded from the sentence. |
| `err_lat` | `double` | `0.0` | Latitude error estimate decoded from GBS. |
| `err_lon` | `double` | `0.0` | Longitude error estimate decoded from GBS. |
| `err_alt` | `double` | `0.0` | Altitude error estimate decoded from GBS. |
| `probability` | `double` | `0.0` | Failed-satellite probability field from GBS. |
| `bias` | `double` | `0.0` | Failed-satellite bias estimate from GBS. |
| `std_dev` | `double` | `0.0` | Failed-satellite standard-deviation estimate from GBS. |
| `failed_sv_id` | `int` | `-1` | Failed satellite identifier; -1 is used when the field is absent. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/gbs.hpp"

using castle::protocols::nmea::message_view;
using castle::protocols::nmea::messages::gbs;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
gbs value{};

if (gbs::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
