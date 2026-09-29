# Distance

## Overview
Great-circle and ellipsoidal distance/bearing calculations over `geo_point<T>`. Provides fast, spherical-earth approximations (`haversine_distance`, `initial_bearing`, `final_bearing`) as well as iterative, ellipsoidal solutions to the geodetic inverse and direct problems (`vincenty_inverse`, `vincenty_direct`) accurate to millimeters on the configured reference ellipsoid.

## Header
`#include "castle_ext/geodetic/distance.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/constants.md`](../../core/constants.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)
- [`../../math/abs.md`](../../math/abs.md)
- [`../../math/sqrt_real.md`](../../math/sqrt_real.md)
- [`../../math/linalg/trigonometry.md`](../../math/linalg/trigonometry.md)
- [`coord.md`](coord.md)
- [`geo_utils.md`](geo_utils.md)
- [`datum.md`](datum.md)

## Public API
| API | Description |
|---|---|
| `haversine_distance<T, Ellipsoid = wgs84_ellipsoid<T>>(from, to)` | Great-circle distance between two points on a sphere, in meters, using the Haversine formula. |
| `initial_bearing<T>(from, to)` | Initial (forward) great-circle bearing from `from` to `to`, in decimal degrees normalized to `[0, 360)`. |
| `final_bearing<T>(from, to)` | Final (arrival) great-circle bearing from `from` to `to`, in decimal degrees normalized to `[0, 360)`. |
| `vincenty_result<T>` | Bundles the outputs of `vincenty_inverse()`: `distance`, `initial_bearing_deg`, `final_bearing_deg`. |
| `vincenty_inverse<T, Ellipsoid = wgs84_ellipsoid<T>>(from, to, result, max_iterations = 200)` | Solves the geodetic inverse problem (distance and bearings between two points) on a reference ellipsoid; returns `castle::status::ok` on convergence, `castle::status::unknown_error` otherwise. |
| `vincenty_direct<T, Ellipsoid = wgs84_ellipsoid<T>>(start, initial_bearing_deg, distance_m, destination, final_bearing_deg, max_iterations = 200)` | Solves the geodetic direct problem (destination point and arrival bearing given a start point, bearing, and distance); returns `castle::status::ok` on convergence, `castle::status::unknown_error` otherwise. |

## Usage Example
See `samples/castle_ext/geodetic/distance.cpp`.

```cpp
#include "castle_ext/geodetic/distance.hpp"

using castle_ext::geodetic::geo_point;
const geo_point<double> hanoi(21.0278, 105.8342);
const geo_point<double> saigon(10.7626, 106.6602);

const double approx_m = castle_ext::geodetic::haversine_distance(hanoi, saigon);

castle_ext::geodetic::vincenty_result<double> precise;
const castle::status result = castle_ext::geodetic::vincenty_inverse(hanoi, saigon, precise);
```

## Constraints & Notes
- Both algorithm families are parameterized on an ellipsoid policy type (see [`datum.md`](datum.md)), so callers can switch reference ellipsoids without touching call sites.
- All functions depend on `<math.h>` trigonometric wrappers, so unlike `coord.hpp`, none of them are `constexpr`.
- `haversine_distance` treats the earth as a perfect sphere; expect up to ~0.5% error relative to an ellipsoidal solution such as `vincenty_inverse()`.
- `vincenty_inverse()`/`vincenty_direct()` leave their output parameters unmodified when they return anything other than `castle::status::ok`, and non-convergence within `max_iterations` is a known limitation of Vincenty's formula for nearly-antipodal points.
- `vincenty_inverse()` reports zero distance and zero bearings for coincident (or antipodal-degenerate) points instead of iterating.
