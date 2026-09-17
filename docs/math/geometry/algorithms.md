# Geometry algorithms

## Overview
Distance, projection, containment, and convex-hull helpers for Castle geometry types. Use these algorithms when you need deterministic geometry queries over fixed-size points, lines, and polygons.

## Header
`#include "castle/math/geometry/algorithms.hpp"`

## Dependencies
- [`../../container/array.md`](../../container/array.md)
- [`detail.md`](detail.md)
- [`line2d.md`](line2d.md)
- [`line3d.md`](line3d.md)
- [`polygon2d.md`](polygon2d.md)
- [`../hypot.md`](../hypot.md)
- [`../sqrt_real.md`](../sqrt_real.md)
- [`../square.md`](../square.md)

## Public API
| API | Description |
|---|---|
| `squared_distance(point2d<T>, point2d<T>)` | Returns squared 2D Euclidean distance without a square root. |
| `squared_distance(point3d<T>, point3d<T>)` | Returns squared 3D Euclidean distance without a square root. |
| `distance(point2d<T>, point2d<T>)` | Floating-point 2D Euclidean distance. |
| `distance(point3d<T>, point3d<T>)` | Floating-point 3D Euclidean distance. |
| `nearest_point(point, line)` | Projects a point onto an infinite 2D or 3D line. |
| `nearest_point_on_segment(point, first, second)` | Projects a point onto a closed 2D or 3D segment. |
| `nearest_point_result2d<T>` / `nearest_point_result3d<T>` | Result structs carrying `found`, `index`, `point`, and `distance_squared`. |
| `nearest_point(array, target)` | Linear nearest-neighbor search over a fixed `container::array`. |
| `point_on_segment(point, first, second, epsilon)` | Segment membership test with epsilon tolerance. |
| `point_in_polygon(point, vertices, count, epsilon)` | Ray-casting point-in-polygon test; boundary counts as inside. |
| `point_in_polygon(point, polygon, epsilon)` | Convenience overload for `polygon2d`. |
| `convex_hull(input, output)` | Monotone-chain convex hull into a fixed-capacity `polygon2d`. |

## Usage Example
See `samples/sample_algorithms.cpp`.

```cpp
#include "castle/math/geometry/algorithms.hpp"

castle::math::point2d<int> a(0, 0);
castle::math::point2d<int> b(3, 4);
auto d2 = castle::math::squared_distance(a, b);
```

## Constraints & Notes
- `distance`, `nearest_point`, and `nearest_point_on_segment` require floating-point coordinate types where enforced by `static_assert`.
- `point_in_polygon` and `convex_hull` require signed or floating-point coordinates.
- `point_in_polygon` treats boundary points as inside and uses the supplied vertex order directly; no convexity assumption is required.
- `convex_hull` removes duplicate points and discards collinear points that lie strictly between hull extremes.
- Selected integer-heavy routines use `geometry_calc_type` to widen intermediate determinants/areas.
