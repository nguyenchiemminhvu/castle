# Geo

## Overview
Geographic coordinate angles for `castle::units`: latitude and longitude in radians, degrees, u-blox 1e-7 degrees, Garmin semicircles, arc-seconds and milliarc-seconds. They carry a kind tag, so a `geo_degrees<double>` latitude cannot be mixed implicitly with a plane-angle `degrees<double>` heading; `quantity_cast` converts explicitly.

## Header
`#include "castle/units/geo.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [angle](angle.md) (optional, for `sin`, `cos`, `wrap_signed` on geographic angles)

## Public API
| API | Description |
|---|---|
| `geo_kind` | Kind tag separating geographic angles from plane angles. |
| `geo_radian_unit` | Coherent geographic angle unit. |
| `geo_degree_unit`, `geo_degree_e7_unit`, `geo_semicircle_unit` | Degree (pi/180 rad), 1e-7 degree (u-blox UBX int32 lat/lon), semicircle (180 deg / 2^31, Garmin FIT). |
| `geo_arcsecond_unit`, `geo_milliarcsecond_unit` | Arc-second (1/3600 deg), milliarc-second (1/3600000 deg). |
| `geo_radians<Rep>`, `geo_degrees<Rep>`, `geo_degrees_e7<Rep>`, `geo_semicircles<Rep>`, `geo_arcseconds<Rep>`, `geo_milliarcseconds<Rep>` | `quantity` aliases for the units. |
| `is_valid_latitude(q)` | True when -90 deg <= q <= 90 deg. |
| `is_valid_longitude(q)` | True when -180 deg <= q <= 180 deg. |
| `literals::_geo_deg`, `_geo_rad`, `_geo_arcsec`, `_geo_mas` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/geo.cpp`.

```cpp
#include "castle/units/angle.hpp"
#include "castle/units/geo.hpp"

using namespace castle::units::literals;

castle::units::geo_degrees_e7<int32_t> raw(375665000);                 // UBX lat
const castle::units::geo_degrees<double> lat = raw;                    // 37.5665 deg
const bool ok = castle::units::is_valid_latitude(raw);                 // true
const auto lon = castle::units::wrap_signed(190.0_geo_deg);            // -170 deg
const auto plane = castle::units::quantity_cast<castle::units::degrees<double>>(lat); // explicit
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- `geo_degrees<double>` is neither implicitly convertible nor constructible from `degrees<double>` (and vice versa); only `quantity_cast` crosses the kind boundary.
- The validators are templates enabled only for angle units tagged with `geo_kind`. They compare in `geo_degrees<double>`, so integral e7, semicircle or arc-second values cannot overflow.
- `geo_milliarcseconds<int32_t>` covers ±180 deg (±648000000 mas) with headroom; converting integral degrees to milliarc-seconds is exact.
- Include [angle](angle.md) to use `sin`, `cos` or `wrap_signed` on geographic angles; the kind is preserved by the wrap helpers.
