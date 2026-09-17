# Least common multiple

## Overview

Integer LCM helper, typically used together with GCD and ratio calculations.

## Header

```cpp
#include "castle/math/lcm.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `lcm(a, b)` | Compute the least common multiple. |
| Compile-time support | Constexpr/template-friendly usage where provided. |

## Example

```cpp
auto n = castle::math::lcm(6, 8); // 24
```

## Embedded notes

- Watch for integer overflow when the result does not fit the chosen type.
