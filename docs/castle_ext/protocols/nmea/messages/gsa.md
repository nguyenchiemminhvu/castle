# NMEA GSA

## Overview

Represents an NMEA GSA DOP/fix-mode sentence. It stores the navigation mode, a fixed-capacity list of up to 12 satellite IDs, DOP values, and the optional system identifier.

## Header

`#include "castle_ext/protocols/nmea/messages/gsa.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../nmea.md)
- [`castle_ext/protocols/nmea/field_cursor.hpp`](../field_cursor.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `gsa` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `gsa::max_satellites` | `static constexpr castle::size_type` protocol/configuration constant (`12U`). |
| `gsa_op_mode` | Strongly typed protocol enumeration used by the decoded model. |
| `gsa_nav_mode` | Strongly typed protocol enumeration used by the decoded model. |
| `gsa::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `gsa::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `gsa` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `valid` | `bool` | `false` | True when decoding completed successfully for the sentence. |
| `talker` | `castle::container::string_view` | `—` | Non-owning NMEA talker identifier view. |
| `op_mode` | `gsa_op_mode` | `gsa_op_mode::unknown` | GSA operational mode enumeration. |
| `nav_mode` | `gsa_nav_mode` | `gsa_nav_mode::unknown` | GSA navigation/fix mode enumeration. |
| `sat_ids` | `castle::container::array<uint8_t,max_satellites>` | `—` | Fixed-capacity satellite-ID array from GSA. |
| `pdop` | `double` | `0.0` | Position dilution of precision. |
| `hdop` | `double` | `0.0` | Horizontal dilution-of-precision value. |
| `vdop` | `double` | `0.0` | Vertical dilution of precision. |
| `system_id` | `uint8_t` | `0U` | Optional NMEA system identifier. |

### Enumerations

#### `gsa_op_mode`

| Enumerator | Value |
| --- | --- |
| `auto_mode` | `'A'` |
| `manual` | `'M'` |
| `unknown` | `'\0'` |
#### `gsa_nav_mode`

| Enumerator | Value |
| --- | --- |
| `no_fix` | `1U` |
| `fix_2d` | `2U` |
| `fix_3d` | `3U` |
| `unknown` | `0U` |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/gsa.hpp"

using castle_ext::protocols::nmea::message_view;
using castle_ext::protocols::nmea::messages::gsa;

message_view raw = /* checksum-valid sentence supplied by nmea_parser */;
gsa value{};

if (gsa::decode(raw, value)) {
    // `value` is now the bounded, typed representation.
}
```

## Constraints & Notes

- Sentence views and string views are non-owning; their referenced parser storage must remain unchanged for the lifetime of the view.
- Decoders are deterministic and perform no dynamic allocation. Optional sentence fields are handled with bounded field traversal and default values.
- The `valid` member is an explicit decode-result indicator; a false return from `decode()` means the output should not be consumed.
- The GSA satellite list is fixed at `max_satellites == 12U`; the decoder does not allocate for a larger list.
