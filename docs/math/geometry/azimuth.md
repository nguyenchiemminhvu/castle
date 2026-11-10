# Geometry azimuth

## Overview
Compass azimuth from one `point2d` or `point3d` to another, in radians or degrees. Coordinates are read in a local East-North-Up (ENU) frame (`x` = East, `y` = North, `z` = Up) and the angle is measured clockwise from North: 0° = North, 90° = East, 180° = South, 270° = West. Typical use: the azimuth of a GNSS satellite seen from a receiver, once the satellite position is expressed in the receiver's local ENU frame.

## Header
`#include "castle/math/geometry/azimuth.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/constants.md`](../../core/constants.md) (`tau`, `radians_to_degrees`)
- [`../../core/traits.md`](../../core/traits.md)
- [`point2d.md`](point2d.md)
- [`point3d.md`](point3d.md)
- [`../linalg/trigonometry.md`](../linalg/trigonometry.md) (`radians_atan2`)

## Public API
| API | Returns | Description |
|---|---|---|
| `T azimuth(const point2d<T>& from, const point2d<T>& to)` | radians in `[0, 2π)` | `atan2(dx, dy)` wrapped into one turn. |
| `T azimuth_degrees(const point2d<T>& from, const point2d<T>& to)` | degrees in `[0, 360)` | Same angle in degrees. |
| `T azimuth(const point3d<T>& from, const point3d<T>& to)` | radians in `[0, 2π)` | Azimuth of the horizontal (`x`, `y`) displacement; `z` is ignored. |
| `T azimuth_degrees(const point3d<T>& from, const point3d<T>& to)` | degrees in `[0, 360)` | Same angle in degrees. |
| `detail::wrap_azimuth(angle, full_turn)` | `[0, full_turn)` | Internal: wraps an `atan2` result (radians or degrees) into one turn. |
| `detail::signed_azimuth(east, north)` | radians in `[-π, π]` | Internal: `atan2(east, north)` with a zero-displacement guard. |

`dx = to.x() - from.x()`, `dy = to.y() - from.y()`. `T` must be a floating-point type (`float`, `double`, `long double`).

Complexity: O(1), one `atan2` call.

## Usage Example
See `samples/math/geometry/azimuth.cpp`.

```cpp
#include "castle/math/geometry/azimuth.hpp"

using point_type = castle::math::point2d<double>;

point_type p1(0.0, 0.0);
point_type p2(1.0, 1.0);

double rad = castle::math::azimuth(p1, p2);          // 0.785398 (pi / 4)
double deg = castle::math::azimuth_degrees(p1, p2);  // 45.0

castle::math::point3d<double> receiver(0.0, 0.0, 0.0);
castle::math::point3d<double> satellite_enu(-1.1e7, -1.1e7, 1.5e7);
double sat_az = castle::math::azimuth_degrees(receiver, satellite_enu); // 225.0 (south-west)
```

## Constraints & Notes
- Formula matches Boost.Geometry's cartesian `azimuth` (`atan2(dx, dy)`), but the result is wrapped to `[0, 2π)` instead of Boost's `(-π, π]`, so West is 270° rather than -90°.
- Coincident points, or a 3D target straight above/below the observer, have no defined heading; the functions return exactly `0`. This guard also avoids `atan2(0, 0)` (a domain error on some C libraries, `-π` for negative zeros on IEEE platforms).
- The degree overload converts the signed `atan2` result before wrapping, so neither overload can return the full-turn value (`2π` / `360`) through rounding; a tiny negative angle folds to `0`.
- Input must already be in a local ENU frame. ECEF or geodetic coordinates must be rotated into ENU first; this header does no frame conversion.
- NaN coordinates propagate to a NaN result.
- Precision is that of the platform `atan2f` / `atan2` / `atan2l`. No allocation, no exceptions, reentrant and thread-safe (no shared state).
- Not `constexpr`: the C library `atan2` is not a constant expression.
