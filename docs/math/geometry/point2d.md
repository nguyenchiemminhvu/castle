
# 2D point

## Overview

Represents a position in a two-dimensional coordinate system. A point is kept separate from a vector so point/vector expressions retain geometric meaning.

## Header

```cpp
#include "castle/math/geometry/point2d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `point2d<T>` | Store `x_` and `y_` coordinates. |
| `x()` / `y()` | Read coordinates. |
| `set_x()` / `set_y()` | Modify coordinates. |
| `operator==` / `operator!=` | Compare coordinates exactly. |

## Example

```cpp
castle::math::point2d<int> waypoint(120, 80);
waypoint.set_x(128);
```

## Embedded notes

- `T` must be an arithmetic type.
- The type is `constexpr` constructible and suitable for pixel, map, waypoint, and sensor coordinates.
- Use `point - point` to obtain a vector and `point + vector` to move a position.
