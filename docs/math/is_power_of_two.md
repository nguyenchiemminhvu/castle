
# Power-of-two test

## Overview

Checks whether an integral value is a non-zero power of two using bit arithmetic.

## Header

```cpp
#include "castle/math/is_power_of_two.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `is_power_of_two()` | Return `true` only for exact positive powers of two. |

## Example

```cpp
if (castle::math::is_power_of_two(buffer_size))
{
    // A mask-based ring-buffer index can be used.
}
```

## Embedded notes

- Zero and negative signed values are not powers of two.
- The helper is `constexpr`.
- Typical uses include validating ring-buffer sizes, table dimensions, memory block sizes, and shift-friendly configuration values.
