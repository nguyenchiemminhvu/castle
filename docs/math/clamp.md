# Clamp

## Overview

Restricts a value to a lower and upper bound without requiring STL algorithms.

## Header

```cpp
#include "castle/math/clamp.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `clamp(value, low, high)` | Return `low` when below range, `high` when above range, otherwise the value. |

## Example

```cpp
auto duty = castle::math::clamp(input, 0, 100);
```

## Embedded notes

- Make sure `low <= high` for meaningful results.
