# Swap helper

## Overview

STL-free swap helper for moving values without depending on `<utility>`.

## Header

```cpp
#include "castle/utility/swap.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `swap(a, b)` | Exchange two objects using Castle move semantics. |

## Example

```cpp
int a = 1, b = 2;
castle::swap(a, b);
```

## Embedded notes

- Used internally by tuple, pair and containers.
