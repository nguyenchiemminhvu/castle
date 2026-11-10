# Geometry elevation

## Overview
Elevation angle from one `point3d` to another, in radians or degrees. Coordinates are read in a local East-North-Up (ENU) frame (`x` = East, `y` = North, `z` = Up) and the angle is measured upward from the local horizon (the `xy` plane): 0° = on the horizon, 90° = zenith, -90° = nadir. Typical use: GNSS satellite elevation and elevation-mask filtering once the satellite position is expressed in the receiver's local ENU frame.

## Header
`#include "castle/math/geometry/elevation.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/constants.md`](../../core/constants.md) (`radians_to_degrees`)
- [`../../core/traits.md`](../../core/traits.md)
- [`point3d.md`](point3d.md)
- [`../hypot.md`](../hypot.md)
- [`../linalg/trigonometry.md`](../linalg/trigonometry.md) (`radians_atan2`)

## Public API
| API | Returns | Description |
|---|---|---|
| `T elevation(const point3d<T>& from, const point3d<T>& to)` | radians in `[-π/2, π/2]` | `atan2(dz, hypot(dx, dy))`. |
| `T elevation_degrees(const point3d<T>& from, const point3d<T>& to)` | degrees in `[-90, 90]` | Same angle in degrees. |

`dx`, `dy`, `dz` are `to - from` per axis. `T` must be a floating-point type (`float`, `double`, `long double`).

Complexity: O(1), one overflow-resistant `hypot` and one `atan2` call.

## Usage Example
See `samples/math/geometry/elevation.cpp`.

```cpp
#include "castle/math/geometry/elevation.hpp"

castle::math::point3d<double> receiver(0.0, 0.0, 0.0);
castle::math::point3d<double> satellite_enu(3.0e6, 4.0e6, 5.0e6);

double rad = castle::math::elevation(receiver, satellite_enu);         // 0.785398 (pi / 4)
double deg = castle::math::elevation_degrees(receiver, satellite_enu); // 45.0

bool visible = deg >= 10.0; // 10 degree elevation mask
```

## Constraints & Notes
- 2D points have no vertical axis, so their elevation is identically zero; no `point2d` overload is provided. Use [`azimuth.md`](azimuth.md) for the horizontal direction of 2D and 3D points.
- Coincident points return exactly `0`; this guard avoids `atan2(0, 0)`, a domain error on some C libraries. A target straight above or below returns `±π/2` (`±90°`).
- The horizontal distance uses `castle::math::hypot`, which scales before squaring, so large ENU offsets (satellite ranges ~2·10⁷ m) do not overflow `float` intermediates.
- Input must already be in a local ENU frame. ECEF or geodetic coordinates must be rotated into ENU first; this header does no frame conversion.
- NaN coordinates propagate to a NaN result.
- Precision is that of the platform `atan2f` / `atan2` / `atan2l`. No allocation, no exceptions, reentrant and thread-safe (no shared state).
- Not `constexpr`: the C library `atan2` is not a constant expression.
