# Math umbrella header

## Overview

Convenience include for Castle math utilities.

## Header

```cpp
#include "castle/math/math.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Math helpers | Includes the scalar math, ratio, random, root, mean, GCD/LCM and related headers. |

## Example

```cpp
#include "castle/math/math.h"
auto x = castle::math::clamp(120, 0, 100);
```

## Embedded notes

- Prefer feature-specific headers for smaller dependency graphs.
