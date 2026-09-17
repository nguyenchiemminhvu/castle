
# Sign helper

## Overview

Returns the mathematical sign of a value as `-1`, `0`, or `+1`.

## Header

```cpp
#include "castle/math/sign.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `sign(value)` | Return `-1` for negative, `0` for zero, and `+1` for positive values. |

## Example

```cpp
const int direction =
    castle::math::sign(encoder_error);
```

## Embedded notes

- Unsigned types produce only `0` or `+1`.
- The helper is `constexpr`.
- Typical uses include direction selection, steering decisions, and control-state branching.
