
# Infinite 2D line

## Overview

Represents an infinite two-dimensional line as an origin point plus a direction vector.

## Header

```cpp
#include "castle/math/geometry/line2d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `line2d(origin, direction)` | Construct a parameterized line. |
| `line2d(first, second)` | Construct a line through two points. |
| `origin()` / `direction()` | Access the line representation. |
| `point_at(parameter)` | Evaluate `origin + direction * parameter`. |
| `degenerate()` | Detect a zero direction. |

## Example

```cpp
castle::math::line2d<float> path(
    castle::math::point2d<float>(0.0f, 0.0f),
    castle::math::point2d<float>(10.0f, 5.0f));

const auto midpoint = path.point_at(0.5f);
```

## Embedded notes

- The line is infinite; a segment needs explicit endpoint handling.
- A zero direction produces a degenerate line and should be treated separately by geometric algorithms.
