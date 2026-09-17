# Geometry aggregate header

## Overview
Aggregate include for all Castle geometry building blocks. Use it when you want the full geometry surface from the `castle::math` namespace without spelling every individual header.

## Header
`#include "castle/math/geometry/geometry.hpp"`

## Dependencies
- [`algorithms.md`](algorithms.md)
- [`circle2d.md`](circle2d.md)
- [`circle3d.md`](circle3d.md)
- [`intersections.md`](intersections.md)
- [`line2d.md`](line2d.md)
- [`line3d.md`](line3d.md)
- [`plane3d.md`](plane3d.md)
- [`point2d.md`](point2d.md)
- [`point3d.md`](point3d.md)
- [`polygon2d.md`](polygon2d.md)
- [`vector2d.md`](vector2d.md)
- [`vector3d.md`](vector3d.md)

## Public API
This header has no direct declarations. It re-exports all geometry primitives, algorithms, and intersection helpers listed above.

## Usage Example
See `samples/sample_geometry_all.cpp`.

```cpp
#include "castle/math/geometry/geometry.hpp"

castle::math::circle2d<float> c(castle::math::point2d<float>(0.0f, 0.0f), 5.0f);
auto hits = castle::math::intersect_line_circle(
    castle::math::line2d<float>(castle::math::point2d<float>(-6.0f, 0.0f), castle::math::vector2d<float>(1.0f, 0.0f)),
    c);
```

## Constraints & Notes
- Header-only aggregate.
- Prefer individual headers when dependency size matters.
- All included components remain deterministic and allocation-free.
