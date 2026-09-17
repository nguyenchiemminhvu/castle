# Integer inversion helpers

## Overview

Helpers for computing multiplicative or bit-oriented inverses used by low-level integer code.

## Header

```cpp
#include "castle/math/invert.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Invert helper | Compute the inverse form defined by this header for the supported integer type. |
| Compile-time form | Use the constexpr/template helper for fixed constants where provided. |

## Example

```cpp
// Consult the function/template signature in invert.h for the exact supported inversion domain.
```

## Embedded notes

- This header is intended for deterministic integer math, not floating-point numerical inversion.
