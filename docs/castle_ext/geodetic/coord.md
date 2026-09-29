# Coord

## Overview
Geographic coordinate representation and angle-format conversions. Use it for a validated latitude/longitude pair (`geo_point<T>`), structured Degrees-Minutes-Seconds/Degrees-Decimal-Minutes representations, free-function conversions between decimal degrees and other angle units, and an optional NATO/GEOREF-style DMS text formatter. All conversions are pure arithmetic and available at compile time via `constexpr`.

## Header
`#include "castle_ext/geodetic/coord.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/error_handler.md`](../../core/error_handler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../math/math.md`](../../math/math.md)
- [`../../utility/string_builder.md`](../../utility/string_builder.md)
- [`geo_utils.md`](geo_utils.md)

## Public API
| API | Description |
|---|---|
| `geo_point<T>` | Validated decimal-degree latitude/longitude pair, intentionally distinct from `castle::math::point2d` so latitude/longitude semantics stay explicit. |
| `geo_point()` / `geo_point(latitude_deg, longitude_deg)` | Construct the origin or an explicit point; asserts each coordinate is within its valid range. |
| `latitude()`, `longitude()` | Return the stored coordinates. |
| `operator==`, `operator!=` | Component-wise comparison. |
| `dms<T>` | Degrees-Minutes-Seconds angle representation with a `negative` sign flag. |
| `ddm<T>` | Degrees and Decimal-Minutes angle representation with a `negative` sign flag. |
| `degrees_to_dms(decimal_degrees)`, `dms_to_degrees(value)` | Convert between decimal degrees and `dms<T>`. |
| `degrees_to_ddm(decimal_degrees)`, `ddm_to_degrees(value)` | Convert between decimal degrees and `ddm<T>`. |
| `degrees_to_arcseconds(v)`, `arcseconds_to_degrees(v)` | Convert between decimal degrees and arcseconds. |
| `degrees_to_milliarcseconds(v)`, `milliarcseconds_to_degrees(v)` | Convert between decimal degrees and milliarcseconds. |
| `degrees_to_microarcseconds(v)`, `microarcseconds_to_degrees(v)` | Convert between decimal degrees and microarcseconds. |
| `degrees_to_gradians(v)`, `gradians_to_degrees(v)` | Convert between decimal degrees and gradians (gon). |
| `degrees_to_turns(v)`, `turns_to_degrees(v)` | Convert between decimal degrees and turns (full rotations). |
| `append_dms(builder, value, positive_symbol, negative_symbol, precision)` | Appends a NATO/GEOREF-style DMS string (e.g. `51 28'39.383"N`) to a `castle::basic_string_builder`. |
| `append_dms_lat(builder, value, precision)` | Convenience wrapper appending a latitude DMS string with `N`/`S`. |
| `append_dms_lon(builder, value, precision)` | Convenience wrapper appending a longitude DMS string with `E`/`W`. |

## Usage Example
See `samples/castle_ext/geodetic/coord.cpp`.

```cpp
#include "castle_ext/geodetic/coord.hpp"

constexpr castle::geodetic::geo_point<double> hanoi(21.0278, 105.8342);
constexpr castle::geodetic::dms<double> lat_dms =
    castle::geodetic::degrees_to_dms(hanoi.latitude());
```

## Constraints & Notes
- `geo_point<T>` and every conversion function require a floating-point `T`, enforced by `static_assert`.
- Out-of-range latitude/longitude arguments to `geo_point`'s constructor violate the precondition and trigger Castle's error handling when checks are enabled; the value is still stored unchanged either way.
- Degrees/radians conversions intentionally live in `castle::math` (`degrees_to_radians`, `radians_to_degrees`, from `castle/core/constants.hpp`) and are not duplicated here.
- `dms`/`ddm` carry the sign in a separate `negative` field so `degrees`/`minutes`/`seconds` stay non-negative, matching conventional DMS notation with N/S/E/W hemisphere letters.
- `append_dms*` overflow handling is delegated to the string builder itself (truncation with `...`).
