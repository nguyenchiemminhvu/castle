# Integer logarithms

## Overview

Helpers for computing small integer logarithm/power relationships without floating-point libraries.

## Header

```cpp
#include "castle/math/logarithm.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Logarithm helpers | Compute integer base-2 or related logarithms supported by the header. |
| Compile-time forms | Useful for table and buffer sizing. |

## Example

```cpp
auto bits = castle::math::log2(256u);
```

## Embedded notes

- Check the header signature for the supported base and zero-input policy.
