
# 2D circle

## Overview

Represents a two-dimensional circle by center and non-negative radius.

## Header

```cpp
#include "castle/math/geometry/circle2d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `circle2d(center, radius)` | Construct a circle. |
| `center()` / `radius()` | Access circle parameters. |
| `contains(point, epsilon)` | Test membership in the filled disk. |
| `on_circle(point, epsilon)` | Test membership near the circumference. |

## Example

```cpp
castle::math::circle2d<float> detection_zone(
    castle::math::point2d<float>(5.0f, 5.0f), 2.0f);

if (detection_zone.contains(object_position, 1e-3f))
{
    // Object is inside the zone.
}
```

## Embedded notes

- Radius must be non-negative.
- `contains()` treats the interior and circumference as inside.
- `on_circle()` checks a radial band defined by `epsilon`.
- Typical uses include proximity zones, collision envelopes, and map range rings.
