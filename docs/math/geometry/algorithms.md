
# Geometry algorithms

## Overview

Deterministic geometry algorithms operating on Castle point, line, and fixed-capacity array types.

## Header

```cpp
#include "castle/math/geometry/algorithms.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose | Complexity |
|---|---|---:|
| `squared_distance()` | Compare distances without a square root. | `O(1)` |
| `distance()` | Return Euclidean distance for floating-point points. | `O(1)` |
| `nearest_point(point, line)` | Project onto an infinite line. | `O(1)` |
| `nearest_point_on_segment()` | Project and clamp to a segment. | `O(1)` |
| `nearest_point(array, target)` | Linear nearest-neighbor search in a fixed array. | `O(N)` |
| `point_on_segment()` | Test whether a point lies on a 2D segment. | `O(1)` |
| `point_in_polygon()` | Ray-casting/Jordan-curve point containment. | `O(N)` |
| `convex_hull()` | Build a hull with monotone chain. | `O(N log N)` |

## Example

```cpp
castle::container::array<
    castle::math::point2d<int>, 5> samples;

samples[0] = castle::math::point2d<int>(10, 10);
samples[1] = castle::math::point2d<int>(2, 3);
samples[2] = castle::math::point2d<int>(7, 9);
samples[3] = castle::math::point2d<int>(4, 4);
samples[4] = castle::math::point2d<int>(20, 20);

const auto nearest =
    castle::math::nearest_point(
        samples, castle::math::point2d<int>(5, 5));
```

## Convex hull

`convex_hull()` uses the monotone-chain algorithm with a local iterative heap sort. It keeps the workspace in fixed Castle arrays and removes duplicate points and collinear interior edge points.

```cpp
castle::math::polygon2d<int, 6> hull;
castle::math::convex_hull(samples, hull);
```

## Point-in-polygon

The polygon test uses the ray-casting form of the Jordan curve theorem. Boundary points are classified as inside.

The implementation uses a division-free crossing decision inside the `O(N)` loop, which is useful on microcontrollers where avoiding a floating-point divide can matter.

## Embedded notes

- Use squared distance when only ordering is needed; this avoids `sqrt_real()`.
- The fixed-array nearest-point search is deterministic and uses `O(1)` extra storage.
- Continuous line and segment projections require floating-point coordinates.
- `convex_hull()` is appropriate for fixed-size point clouds and collision envelopes where heap allocation is undesirable.
