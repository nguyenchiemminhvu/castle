# Comparison helpers

## Overview

STL-free comparison functors and helpers used by ordered containers and generic code.

## Header

```cpp
#include "castle/utility/compare.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `less<T>` / `greater<T>` | Ordering predicates. |
| Equality predicates | Equal/not-equal style helpers where provided. |
| Three-way-style helpers | Convenience comparisons used by Castle generic algorithms. |

## Example

```cpp
castle::less<int> less;
bool before = less(2, 3);
```

## Embedded notes

- Use Castle comparison functors when keeping the dependency graph free of `<functional>`.
