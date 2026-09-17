# Intersections

## Overview
Relation tests and point-returning intersection helpers for Castle lines, planes, and circles. Use this header when you need deterministic intersection classification with fixed-size return storage.

## Header
`#include "castle/math/geometry/intersections.hpp"`

## Dependencies
- [`circle2d.md`](circle2d.md)
- [`circle3d.md`](circle3d.md)
- [`detail.md`](detail.md)
- [`line2d.md`](line2d.md)
- [`line3d.md`](line3d.md)
- [`plane3d.md`](plane3d.md)
- [`vector3d.md`](vector3d.md)
- [`../../container/array.md`](../../container/array.md)
- [`../../core/type_ranges.md`](../../core/type_ranges.md)
- [`../../core/types.md`](../../core/types.md)
- [`../sqrt_real.md`](../sqrt_real.md)

## Public API
| API | Description |
|---|---|
| `line2d_relation`, `line3d_relation`, `line_plane_relation`, `plane3d_relation` | Enumerations describing geometric relationships. |
| `intersection_points2d<T>`, `intersection_points3d<T>` | Result structs holding `count`, up to two points, and a `coincident` flag. |
| `intersect_lines(line2d, line2d, point, epsilon)` | Intersects two infinite 2D lines. |
| `intersect_line_plane(line3d, plane3d, point, epsilon)` | Intersects an infinite 3D line with a plane. |
| `intersect_lines(line3d, line3d, point, epsilon)` | Intersects two infinite 3D lines and distinguishes skew lines. |
| `intersect_planes(plane3d, plane3d, line3d, epsilon)` | Intersects two infinite 3D planes. |
| `intersect_circles(circle2d, circle2d, epsilon)` | Computes up to two 2D circle intersection points. |
| `intersect_line_circle(line2d, circle2d, epsilon)` | Computes up to two 2D line/circle intersection points. |
| `intersect_line_circle(line3d, circle3d, epsilon)` | Computes up to two 3D line/circle intersection points. |
| `intersect_circles(circle3d, circle3d, epsilon)` | Computes up to two 3D circle/circle intersection points. |

## Usage Example
See `samples/sample_intersections.cpp`.

```cpp
#include "castle/math/geometry/intersections.hpp"

castle::math::point2d<float> hit;
auto relation = castle::math::intersect_lines(
    castle::math::line2d<float>(castle::math::point2d<float>(0.0f, 0.0f), castle::math::vector2d<float>(1.0f, 0.0f)),
    castle::math::line2d<float>(castle::math::point2d<float>(0.0f, -1.0f), castle::math::vector2d<float>(0.0f, 1.0f)),
    hit);
```

## Constraints & Notes
- All intersection solver functions require floating-point coordinate types where enforced by `static_assert`.
- Circle-related result structs store at most two explicit points; coincident geometries set `coincident = true` and leave `count = 0`.
- `intersect_lines(line3d, ...)` distinguishes `parallel`, `coincident`, `intersecting`, and `skew`.
- `intersect_line_circle(line3d, circle3d, ...)` handles the cases where the line crosses the plane once, lies entirely in the plane, or is parallel to it.
