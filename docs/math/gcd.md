# Greatest common divisor

## Overview

Integer GCD helper used by ratio and other math utilities.

## Header

```cpp
#include "castle/math/gcd.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `gcd(a, b)` | Compute the greatest common divisor. |
| Compile-time support | Available in constexpr/template-friendly form where provided. |

## Example

```cpp
auto g = castle::math::gcd(84, 30); // 6
```

## Embedded notes

- Useful for reducing ratios and integer scaling factors.
