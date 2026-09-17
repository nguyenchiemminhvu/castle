# Fixed-size mathematical vector

## Overview

A fixed-dimension mathematical vector for embedded numeric and geometry work. Elements are stored directly inside the object, so the size and memory footprint are known at compile time.

## Header

```cpp
#include "castle/math/linalg/vector.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `vector<T, N>` | Fixed-size vector with `N` arithmetic elements. |
| `operator[]` / `at()` / `data()` | Element access and contiguous storage access. |
| `operator+` / `operator-` | Vector addition, subtraction, unary plus and unary minus. |
| Scalar operators | Multiply or divide every element by a scalar. |
| `dot()` / `hadamard()` | Dot product and component-wise multiplication. |
| `length()` / `squared_length()` | Euclidean magnitude. `length()` is available for floating-point vectors. |
| `normalized()` | Return a unit-length vector. The zero vector is rejected by Castle's assertion mechanism. |
| `distance_to()` / `lerp_to()` | Floating-point distance and linear interpolation. |
| `project()` / `reflect()` | Projection onto a basis vector and reflection about a normal. |
| `cross()` / `scalar_triple_product()` | Three-dimensional vector products. |
| `component_min()` / `component_max()` / `abs()` | Component-wise numeric helpers. |
| `near_equal()` | Epsilon-based comparison for floating-point vectors. |
| `vector2` / `vector3` / `vector4` | Common dimension aliases. |

## Example

```cpp
using castle::math::vector;

const vector<float, 3U> velocity(1.0F, 2.0F, 3.0F);
const vector<float, 3U> normal(0.0F, 0.0F, 1.0F);

const float speed = velocity.length();
const vector<float, 3U> direction = velocity.normalized();
const vector<float, 3U> reflected = castle::math::reflect(velocity, normal);
```

## Embedded notes

- Storage is inline; no dynamic allocation is used.
- `vector<T, N>` accepts arithmetic element types and keeps the dimension in the type.
- `length()`, `normalized()`, `distance_to()`, `lerp_to()` and `near_equal()` are floating-point operations.
- `project()` assumes a non-zero basis vector; `reflect()` expects the supplied normal to have the magnitude required by the desired reflection formula.
- `at()` and invalid numeric operations use Castle's assertion/error handling instead of exceptions.
