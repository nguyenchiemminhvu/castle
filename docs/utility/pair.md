# Pair

## Overview

Small two-value aggregate used by maps, trees, and application code.

## Header

```cpp
#include "castle/utility/pair.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `pair<T1, T2>` | Stores `first` and `second`. |
| Constructors/assignment | Copy and move construction/assignment. |
| `make_pair()` | Convenience factory. |
| Comparisons | Pair comparisons where supplied by the header. |

## Example

```cpp
auto p = castle::make_pair(10, 20);
int a = p.first;
int b = p.second;
```

## Embedded notes

- The type is intentionally close to `std::pair` while staying inside the Castle dependency set.
