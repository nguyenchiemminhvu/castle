# Transform

## Overview
Coordinate datum transformation between WGS-84 and GCJ-02 ("Mars Coordinates"), the obfuscated coordinate system mandated for public mapping services within mainland China. Implements the widely used non-linear offset algorithm (as published by, e.g., the `eviltransform` project) plus an iterative numerical inverse, since the forward transform has no closed-form inverse.

## Header
`#include "castle_ext/geodetic/transform.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/constants.md`](../../core/constants.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../math/abs.md`](../../math/abs.md)
- [`../../math/sqrt_real.md`](../../math/sqrt_real.md)
- [`../../math/linalg/trigonometry.md`](../../math/linalg/trigonometry.md)
- [`coord.md`](coord.md)
- [`geo_utils.md`](geo_utils.md)
- [`datum.md`](datum.md)

## Public API
| API | Description |
|---|---|
| `is_in_gcj02_region<T>(point)` | Tests whether a point falls within mainland China's bounding box, as published for the GCJ-02 algorithm. |
| `wgs84_to_gcj02<T>(wgs84_point)` | Converts a WGS-84 point to GCJ-02, or returns `wgs84_point` unchanged when it falls outside `is_in_gcj02_region()`. |
| `gcj02_to_wgs84<T, Iterations = 3>(gcj02_point)` | Converts a GCJ-02 point back to WGS-84 by numerically refining an initial guess, or returns `gcj02_point` unchanged when it falls outside `is_in_gcj02_region()`. |

## Usage Example
See `samples/castle_ext/geodetic/transform.cpp`.

```cpp
#include "castle_ext/geodetic/transform.hpp"

using castle::geodetic::geo_point;
const geo_point<double> beijing_wgs84(39.8946, 116.291);
const geo_point<double> beijing_gcj02 = castle::geodetic::wgs84_to_gcj02(beijing_wgs84);
const geo_point<double> round_trip = castle::geodetic::gcj02_to_wgs84(beijing_gcj02);
```

## Constraints & Notes
- Like [`distance.md`](distance.md), this header depends on `<math.h>` trigonometric wrappers and is therefore not `constexpr`.
- `wgs84_to_gcj02()`/`gcj02_to_wgs84()` require a floating-point `T`, enforced by `static_assert`.
- GCJ-02 has no closed-form inverse; `gcj02_to_wgs84()` refines an initial guess by repeatedly re-encoding it with `wgs84_to_gcj02()` and correcting for the residual offset, which converges quickly because the offset varies smoothly and is bounded to a few hundred meters. The default of three iterations converges to sub-centimeter residuals for every point inside the GCJ-02 region.
- `Iterations` must be at least one, enforced by `static_assert`.
- The offset polynomials use the Krasovsky (1940) ellipsoid (see [`datum.md`](datum.md)), which is specific to the GCJ-02 algorithm and not a general-purpose geodetic datum.
