
# Fixed-capacity 2D polygon

## Overview

Stores an ordered sequence of two-dimensional vertices in a `castle::container::array` without heap allocation.

## Header

```cpp
#include "castle/math/geometry/polygon2d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `polygon2d<T, Capacity>` | Fixed-capacity polygon container. |
| `size()` / `capacity()` / `empty()` / `full()` | Inspect current and maximum vertex counts. |
| `push_back()` / `pop_back()` / `clear()` | Mutate the vertex sequence with Castle status results. |
| `operator[]` / `data()` | Access stored vertices. |
| `begin()` / `end()` | Iterate over the logical vertex sequence. |
| `signed_area2()` | Return twice the signed polygon area. |
| `clockwise()` | Test for clockwise vertex order. |

## Example

```cpp
castle::math::polygon2d<int, 8> room;
room.push_back(castle::math::point2d<int>(0, 0));
room.push_back(castle::math::point2d<int>(10, 0));
room.push_back(castle::math::point2d<int>(10, 6));
room.push_back(castle::math::point2d<int>(0, 6));
```

## Embedded notes

- Storage is embedded in the polygon object.
- `signed_area2()` avoids an unnecessary division when only orientation or doubled area is required.
- Vertices are expected to be supplied in boundary order.
- Typical uses include map zones, keep-out regions, collision masks, and field-of-view polygons.
