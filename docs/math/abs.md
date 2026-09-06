# Absolute value

## Overview

Small absolute-value helpers for integer/floating-point friendly code.

## Header

```cpp
#include "castle/math/abs.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `abs()` | Return the absolute value of a supported scalar type. |
| Compile-time overloads | The implementation is constexpr-friendly where the type permits it. |

## Example

```cpp
auto x = castle::math::abs(-42);
```

## Embedded notes

- Be aware of the minimum signed integer edge case when the absolute value cannot be represented in the same type.
