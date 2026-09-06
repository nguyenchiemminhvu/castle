# Square root

## Overview

Small square-root helpers intended for embedded builds that may not want a general-purpose math runtime.

## Header

```cpp
#include "castle/math/sqrt.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `sqrt()` | Compute square root using the deterministic algorithm provided by the header. |
| Compile-time form | Constexpr/template use is provided for supported input types. |

## Example

```cpp
auto r = castle::math::sqrt(81u);
```

## Embedded notes

- Check the supported input domain and precision in the header before using it for control algorithms.
