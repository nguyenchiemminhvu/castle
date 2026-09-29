# Datum

## Overview
Reference ellipsoid and datum constants for the geodetic module. Values are exposed as `static constexpr` accessors on small, empty policy types so `distance.hpp` and `transform.hpp` algorithms can be parameterized on the ellipsoid via a template argument.

## Header
`#include "castle_ext/geodetic/datum.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/traits.md`](../../core/traits.md)

## Public API
| API | Description |
|---|---|
| `wgs84_ellipsoid<T>` | WGS-84 reference ellipsoid (GPS and every other GNSS constellation after datum transformation); the default ellipsoid policy. Exposes `semi_major_axis()`, `flattening()`, `semi_minor_axis()`, `eccentricity_squared()`. |
| `pz90_ellipsoid<T>` | PZ-90.11 reference ellipsoid used by the Russian GLONASS constellation. Exposes the same four accessors as `wgs84_ellipsoid`. |
| `krasovsky_ellipsoid<T>` | Krasovsky (1940) ellipsoid used by the GCJ-02 obfuscation algorithm. Exposes `semi_major_axis()` and `eccentricity_squared()` only. |
| `gcj02_region<T>` | Geographic bounding box outside of which GCJ-02 obfuscation does not apply. Exposes `min_longitude()`, `max_longitude()`, `min_latitude()`, `max_latitude()`. |

## Usage Example
See `samples/castle_ext/geodetic/datum.cpp`.

```cpp
#include "castle_ext/geodetic/datum.hpp"

constexpr double a = castle_ext::geodetic::wgs84_ellipsoid<double>::semi_major_axis();
constexpr double f = castle_ext::geodetic::wgs84_ellipsoid<double>::flattening();
constexpr double b = castle_ext::geodetic::wgs84_ellipsoid<double>::semi_minor_axis();
```

## Constraints & Notes
- Every policy type requires a floating-point `T`, enforced by `static_assert`.
- All members are `static constexpr` functions rather than data members, so the policy types remain empty, trivially constructible, and usable purely as compile-time template arguments.
- `krasovsky_ellipsoid` intentionally omits `semi_minor_axis()`: the GCJ-02 offset formulas only need `semi_major_axis()` and `eccentricity_squared()`; derive the semi-minor axis as `castle::math::sqrt_real(a * a * (1 - e^2))` if needed.
