
# Saturating integer arithmetic

## Overview

Adds and subtracts integral values while clamping results to the representable range instead of allowing wrap-around.

## Header

```cpp
#include "castle/math/saturating.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `saturating_add(a, b)` | Add two integers and clamp overflow. |
| `saturating_sub(a, b)` | Subtract two integers and clamp underflow/overflow. |

## Example

```cpp
const uint16_t command =
    castle::math::saturating_add(current_command, increment);
```

## Embedded notes

- Both signed and unsigned integral overloads are provided.
- Saturation is often safer than wrap-around for physical commands, counters, and accumulators.
- The helpers are `constexpr`.
