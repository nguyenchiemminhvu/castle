# Geometry umbrella header

## Overview
Convenience include for the entire Castle geometry module. Use it when a translation unit needs several 2D/3D primitives, algorithms, or intersection helpers and minimizing include count matters more than pulling in only one component.

## Header
`#include "castle/math/geometry.hpp"`

## Dependencies
- [`geometry/geometry.md`](geometry/geometry.md)

## Public API
This header has no direct declarations. It re-exports the complete geometry aggregate header.

## Usage Example
See `samples/sample_geometry.cpp`.

```cpp
#include "castle/math/geometry.hpp"

castle::math::point2d<int> a(1, 2);
castle::math::point2d<int> b(4, 6);
castle::math::vector2d<int> delta = b - a;
```

## Constraints & Notes
- Header-only convenience wrapper.
- Pulls in all geometry primitives, algorithms, and intersections.
- No dynamic allocation, exceptions, RTTI, virtual functions, or STL dependencies are introduced by this wrapper itself.
