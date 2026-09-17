
# Square helper

## Overview

A small arithmetic helper for `value * value`.

## Header

```cpp
#include "castle/math/square.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `square(value)` | Return the value multiplied by itself. |

## Example

```cpp
const int squared =
    castle::math::square(error);
```

## Embedded notes

- The result keeps the input arithmetic type.
- Overflow behavior therefore follows the selected integral type.
- The helper is `constexpr` and useful in small fixed-point or compile-time expressions.
