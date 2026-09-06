# Iterator operations

## Overview

Small STL-independent algorithms for moving iterators and measuring ranges.

## Header

```cpp
#include "castle/iterator/operations.h"
```

**Namespace:** `castle::iterator`

## Main API

| API | Purpose |
|---|---|
| `advance()` | Move an iterator by a signed distance. |
| `distance()` | Measure the distance between two iterators. |
| `next()` / `prev()` | Return an iterator moved forward or backward without changing the original where supported. |

## Example

```cpp
auto it = castle::iterator::next(first, 2);
auto n = castle::iterator::distance(first, last);
```

## Embedded notes

- Algorithm cost depends on the iterator category.
