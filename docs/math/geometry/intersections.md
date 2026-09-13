
# Geometry intersections

## Overview

Analytic, constant-time intersection routines for lines, planes, and circles.

## Header

```cpp
#include "castle/math/geometry/intersections.h"
```

**Namespace:** `castle::math`

## Main API

| API | Result |
|---|---|
| `intersect_lines(line2d, line2d, ...)` | `parallel`, `coincident`, or `intersecting`. |
| `intersect_line_plane()` | `parallel`, `contained`, or `intersecting`. |
| `intersect_lines(line3d, line3d, ...)` | `parallel`, `coincident`, `intersecting`, or `skew`. |
| `intersect_planes()` | `parallel`, `coincident`, or an intersecting 3D line. |
| `intersect_circles(circle2d, circle2d)` | Zero, one, or two points, or coincident circles. |
| `intersect_line_circle(line2d, circle2d)` | Zero, one, or two points. |
| `intersect_line_circle(line3d, circle3d)` | Zero, one, or two points. |
| `intersect_circles(circle3d, circle3d)` | Zero, one, or two points, or coincident circles. |

## Example

```cpp
castle::math::point2d<float> hit;
const auto relation = castle::math::intersect_lines(
    first_line, second_line, hit);

if (relation == castle::math::line2d_relation::intersecting)
{
    // Use hit.
}
```

## Algorithm choices

The implementation uses direct analytic formulas rather than iterative solvers:

- 2D lines use a determinant/cross-product denominator.
- 3D lines use the direction cross product and a shortest-connector test to distinguish true intersections from skew lines.
- Line/plane operations use the parametric plane equation.
- Plane/plane operations construct the intersection line from the cross-product direction and two plane equations.
- 2D circle intersections use the standard center-distance construction.
- 3D circle intersections either reduce through the line of plane intersection or construct an orthonormal basis inside a shared plane.

## Numerical tolerance

Most relation/intersection functions accept an explicit `epsilon`.

Use a tolerance derived from the coordinate units and expected sensor/numerical error rather than relying on a global constant.

## Embedded notes

- All operations are `O(1)` and use fixed storage.
- No matrix, dynamic container, or iterative root finder is required by the intersection routines.
- Floating-point coordinates are required for continuous intersection calculations so division does not silently truncate.
