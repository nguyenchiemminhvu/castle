# Floating-point trigonometry wrappers

## Overview

A small trigonometric interface that keeps the platform C math dependency in one Castle header. The layer is intended to make embedded ports able to replace or adapt the underlying implementation without changing linear-algebra code.

## Header

```cpp
#include "castle/math/linalg/trigonometry.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `sin()` / `cos()` | Floating-point sine and cosine using the C math implementation. |
| `radians_sin()` / `radians_cos()` | Typed wrappers restricted to floating-point element types. |
| `sin_cos_result<T>` | Small aggregate containing both sine and cosine values. |
| `sin_cos()` | Evaluate sine and cosine together and return them as one result. |

## Example

```cpp
const castle::math::sin_cos_result<float> values =
    castle::math::sin_cos(1.57079632679F);

const float sine = values.sine;
const float cosine = values.cosine;
```

## Embedded notes

- The header uses C math functions rather than C++ standard-library facilities.
- `float`, `double` and `long double` overloads are provided for `sin()` and `cos()`.
- Typed wrappers enforce a floating-point element type at compile time.
- Deterministic or hardware-specific trigonometric implementations can be introduced at this boundary without changing quaternion or transform APIs.
- Angle units are radians unless a higher-level API explicitly says degrees.
