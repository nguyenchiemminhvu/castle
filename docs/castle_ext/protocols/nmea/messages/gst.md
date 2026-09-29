# NMEA GST

## Overview

Represents an NMEA GST pseudorange-noise/statistics sentence. It decodes the UTC timestamp, RMS deviation, ellipse terms, orientation, and latitude/longitude/altitude error estimates.

## Header

`#include "castle_ext/protocols/nmea/messages/gst.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/string_view.hpp`](../../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)

## Public API

| API | Description |
| --- | --- |
| `gst` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gst::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `gst::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `gst` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `utc_time` | `double` | `0.0` | UTC time field decoded from the sentence. |
| `rms_dev` | `double` | `0.0` | GST RMS deviation estimate. |
| `semi_major` | `double` | `0.0` | GST semi-major error term. |
| `semi_minor` | `double` | `0.0` | GST semi-minor error term. |
| `orient` | `double` | `0.0` | GST error-ellipse orientation. |
| `lat_err` | `double` | `0.0` | GST latitude error estimate. |
| `lon_err` | `double` | `0.0` | GST longitude error estimate. |
| `alt_err` | `double` | `0.0` | GST altitude error estimate. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/gst.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::gst;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
gst value{};

if (gst::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
