
# 2D vector

## Overview

Represents a two-dimensional direction or displacement and provides dot/cross products and basic vector arithmetic.

## Header

```cpp
#include "castle/math/geometry/vector2d.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `vector2d<T>` | Store `x_` and `y_` vector components. |
| `squared_length()` / `length()` | Return squared or Euclidean magnitude. |
| `dot()` | Return the scalar dot product. |
| `cross()` | Return the 2D scalar cross product. |
| `perpendicular()` | Return a 90-degree perpendicular vector. |
| `normalized()` | Return a unit vector for floating-point vectors. |
| Arithmetic operators | Add, subtract, scale, divide, and negate vectors. |

## Example

```cpp
castle::math::vector2d<float> velocity(3.0f, 4.0f);
const float speed = velocity.length();
```

## Embedded notes

- `length()` and `normalized()` require floating-point `T`.
- For integer geometry, prefer `squared_length()` when only distance ordering is needed.
- Typical uses include planar velocity, image gradients, headings, collision normals, and displacement.
