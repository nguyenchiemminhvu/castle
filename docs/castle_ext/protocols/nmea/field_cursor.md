# NMEA field cursor

## Overview

`field_cursor.hpp` provides a sequential, auto-advancing field reader over a tokenized NMEA sentence (`message_view`). NMEA decoders that index `message_view::field(i)` directly can easily introduce off-by-one errors when handling sentences with many fields. `field_cursor` eliminates manual index bookkeeping by consuming, advancing past, and parsing fields in a single step.

Unlike `ubx::payload_reader`'s sticky-error model, NMEA sentence fields are routinely and legitimately empty (e.g., HDOP or position quality fields omitted when a GPS fix is unavailable). Therefore, `field_cursor` does not latch a global error flag. Instead, each method reports its own success status (`bool`), while optional fields can be safely read with fallback methods (`next_double_or`, `next_uint_or`).

## Header

`#include "castle_ext/protocols/nmea/field_cursor.hpp"`

## Dependencies

* [`core/compiler.hpp`](../../../core/compiler.md)
* [`core/types.hpp`](../../../core/types.md)
* [`container/string_view.hpp`](../../../container/string_view.md)
* [`nmea.hpp`](nmea.md)

## Public API

| API | Description |
| --- | --- |
| `field_cursor(sentence)` | Constructs an auto-advancing reader over a tokenized NMEA `message_view`. |
| `next()` | Returns the current field as a `string_view` and advances the cursor index by 1. |
| `index()` | Returns the index of the field that will be consumed by the next `next_*()` call. |
| `skip(count)` | Advances the cursor past `count` fields (default `1`) without parsing them. |
| `next_double(out)` | Consumes the next field and parses it as a `double`. Returns `bool`. |
| `next_int(out, base)` | Consumes the next field and parses it as an `int` with the specified `base` (default `10`). Returns `bool`. |
| `next_uint(out, base)` | Consumes the next field and parses it as a `uint32_t` with the specified `base` (default `10`). Returns `bool`. |
| `next_char(out)` | Consumes the next field and extracts its character value. Returns `bool`. |
| `next_latlon(out)` | Consumes two sequential fields (coordinate value and N/S or E/W direction indicator) and parses them into decimal degrees (`double`). Returns `bool`. |
| `next_double_or(fallback)` | Consumes an optional field as a `double`, returning `fallback` if the field is empty or unparsable. |
| `next_uint_or(fallback, base)` | Consumes an optional field as a `uint32_t`, returning `fallback` if the field is empty or unparsable. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/field_cursor.hpp"
#include "castle_ext/protocols/nmea/nmea.hpp"

// Assuming `sentence` is a validated message_view (e.g., a GPGGA or GPRMC sentence)
castle::protocols::nmea::field_cursor cursor(sentence);

double utc_time = 0.0;
bool ok = cursor.next_double(utc_time);

double latitude = 0.0;
ok = ok && cursor.next_latlon(latitude); // Consumes coordinate value + N/S indicator

double longitude = 0.0;
ok = ok && cursor.next_latlon(longitude); // Consumes coordinate value + E/W indicator

uint32_t fix_quality = 0U;
ok = ok && cursor.next_uint(fix_quality);

// Optional field (HDOP): returns 0.0 fallback if empty or omitted without failing
double hdop = cursor.next_double_or(0.0);

if (!ok)
{
    // Mandatory fields failed to parse or sentence was truncated
    return false;
}

// Proceed with parsed values...
```

## Constraints & Notes

- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- **Lifetime:** The referenced `message_view` and the underlying sentence buffer it aliases must outlive the `field_cursor` instance.
- **No Sticky Error State:** Unlike UBX's payload reader, individual field parse failures or empty fields do not latch a global failure flag. Callers should chain mandatory field evaluations with `&&` or inspect individual return status codes.
- `next_latlon()` consumes **two** consecutive fields from the sentence (value string and direction character string).
- Optional field accessors (`next_double_or()`, `next_uint_or()`) safely retain the provided `fallback` value if the field is empty or fails to parse.
