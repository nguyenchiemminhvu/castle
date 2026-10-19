# Geodetic

## Overview

Umbrella header for the CASTLE geodetic extension. It includes reference ellipsoid and datum definitions, coordinate representation and validation helpers, distance calculations, datum transformations, geofences, and fixed-depth Geohash base-32 and binary encoding/decoding. Include this header when an application uses several geodetic components; include a narrower header when compile-time dependencies should remain small.

## Header

`#include "castle_ext/geodetic/geodetic.hpp"`

## Dependencies

- [`coord.md`](coord.md)
- [`datum.md`](datum.md)
- [`distance.md`](distance.md)
- [`geo_utils.md`](geo_utils.md)
- [`geofence.md`](geofence.md)
- [`transform.md`](transform.md)
- [`geohash.md`](geohash.md)

The umbrella itself does not add a separate algorithm or runtime state; it forwards the public headers above.

## Public API

The umbrella exposes the combined public API of the included headers:

| Component | Main API |
|---|---|
| Coordinates | `geo_point<T>`, `dms<T>`, `ddm<T>`, and angle conversion/formatting functions. |
| Datum | `wgs84_ellipsoid<T>`, `pz90_ellipsoid<T>`, `krasovsky_ellipsoid<T>`, and `gcj02_region<T>`. |
| Shared helpers | `is_valid_latitude()`, `is_valid_longitude()`, `clamp_latitude()`, and `normalize_longitude()`. |
| Distance | Geodesic and surface-distance functions documented in `distance.md`. |
| Transform | `is_in_gcj02_region()`, `wgs84_to_gcj02()`, and `gcj02_to_wgs84()`. |
| Geofences | Circle, rectangle, and polygon fence types plus `is_inside()`. |
| Geohash | `geohash<Depth, T>` storing base-32, binary, packed-bit, center, and bounds forms; `geohash_bounds`, `geohash_bit_count`, and the `geohash_max_depth` constant. |

See the component documentation linked above for complete signatures, parameters, return values, complexity, and constraints.

## Usage Example

```cpp
#include <assert.h>
#include "castle_ext/geodetic/geodetic.hpp"

int main()
{
    const castle::geodetic::geo_point<double> origin(0.0, 0.0);
    castle::geodetic::geohash<9U> hash;
    assert(hash.encode(origin) == castle::status::ok);

    assert(hash.base32().view().compare(castle::container::string_view("s00000000")) == 0);
    assert(hash.center().latitude() == hash.bounds().center_latitude());
    return 0;
}
```

See `samples/castle_ext/geodetic/geodetic.cpp` for a standalone example.

## Constraints & Notes

- The umbrella inherits the constraints of its component headers. In particular, the extension uses fixed-capacity storage and does not require heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- Including the umbrella pulls in more headers than a focused include. For a minimal dependency surface, use `coord.hpp`, `distance.hpp`, `geofence.hpp`, `transform.hpp`, or `geohash.hpp` directly.
- Geohash coordinates default to `double`; precision is selected at compile time by `geohash<Depth, T>`, with `1 <= Depth <= 12`. Unsupported depths fail compilation. Each initialized geohash stores its cell center and bounds, which are available through `center()` and `bounds()`; the original unquantized coordinate is not recoverable.
- The geodetic algorithms use decimal degrees for latitude and longitude. Their individual APIs document datum assumptions and valid ranges.
