# Geo Utils

## Overview
Shared numeric helpers for the geodetic module. Use it for latitude/longitude range validation, clamping, and longitude normalization instead of duplicating range-reduction logic across `coord.hpp`, `distance.hpp`, and `transform.hpp`.

## Header
`#include "castle_ext/geodetic/geo_utils.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../math/clamp.md`](../../math/clamp.md)

## Public API
| API | Description |
|---|---|
| `is_valid_latitude(latitude_deg)` | Returns `true` when `-90 <= latitude_deg <= 90`. |
| `is_valid_longitude(longitude_deg)` | Returns `true` when `-180 <= longitude_deg <= 180`. |
| `clamp_latitude(latitude_deg)` | Restricts a latitude into the valid `[-90, 90]` range. |
| `normalize_longitude(longitude_deg)` | Wraps a longitude into the half-open interval `[-180, 180)`. |

## Usage Example
See `samples/castle_ext/geodetic/geo_utils.cpp`.

```cpp
#include "castle_ext/geodetic/geo_utils.hpp"

const bool valid = castle_ext::geodetic::is_valid_latitude(51.5);
const double wrapped = castle_ext::geodetic::normalize_longitude(200.0);
```

## Constraints & Notes
- All functions require a floating-point `T` and are `static_assert`-checked accordingly.
- `is_valid_latitude`, `is_valid_longitude`, and `clamp_latitude` are `constexpr`.
- `normalize_longitude` uses `fmod` and is therefore not `constexpr`.
- `normalize_longitude` returns a half-open interval: `-180` maps to itself, `180` wraps to `-180`.
