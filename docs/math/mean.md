# Mean and averaging

## Overview

Small helpers for arithmetic means and bounded averaging in embedded-friendly integer code.

## Header

```cpp
#include "castle/math/mean.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Mean helpers | Compute the average of supplied values or a fixed sequence according to the header API. |
| Accumulator patterns | Support simple bounded statistics without heap allocation. |

## Example

```cpp
auto m = castle::math::mean(10, 20, 30);
```

## Embedded notes

- Choose the accumulator type carefully to avoid overflow when summing many values.
