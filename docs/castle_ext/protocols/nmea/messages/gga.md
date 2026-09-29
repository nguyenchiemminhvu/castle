# NMEA GGA

## Overview

Represents an NMEA GGA fix-data sentence. It decodes UTC time, latitude/longitude, fix quality, satellite count, dilution, altitude, geoid separation, and DGPS metadata.

## Header

`#include "castle_ext/protocols/nmea/messages/gga.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `gga` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gga_fix_quality` | Strongly typed protocol enumeration used by the decoded model. |
| `gga::fix_available()` | Returns true when GGA fix quality is not `invalid`. |
| `gga::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `gga::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `gga` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `utc_time` | `double` | `0.0` | UTC time field decoded from the sentence. |
| `latitude` | `double` | `0.0` | Decoded latitude value. |
| `longitude` | `double` | `0.0` | Decoded longitude value. |
| `fix_quality` | `gga_fix_quality` | `gga_fix_quality::invalid` | NMEA GGA fix-quality enumeration. |
| `num_satellites` | `uint8_t` | `0U` | Number of satellites reported by GGA. |
| `hdop` | `double` | `0.0` | Horizontal dilution-of-precision value. |
| `altitude_msl` | `double` | `0.0` | Altitude above mean sea level as reported by GGA. |
| `geoid_separation` | `double` | `0.0` | Geoid separation reported by GGA/GNS. |
| `dgps_age` | `double` | `-1.0` | Age of differential corrections when present. |
| `dgps_station_id` | `uint16_t` | `0U` | Differential-correction station identifier. |

### Enumerations

#### `gga_fix_quality`

| Enumerator | Value |
| --- | --- |
| `invalid` | `0U` |
| `gps_sps` | `1U` |
| `dgps` | `2U` |
| `pps` | `3U` |
| `rtk_fixed` | `4U` |
| `rtk_float` | `5U` |
| `estimated` | `6U` |
| `manual` | `7U` |
| `simulation` | `8U` |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/gga.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::gga;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
gga value{};

if (gga::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
