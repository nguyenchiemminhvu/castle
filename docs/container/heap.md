# Fixed-capacity heap

## Overview

A binary heap stored in an internal array. Push/emplace and pop maintain the heap property without dynamic allocation.

## Header

```cpp
#include "castle/container/heap.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `heap<T, N, Compare>` | Heap with maximum `N` elements. |
| `emplace()` / `push()` | Add a value and sift it into place. |
| `pop()` | Remove the top value. |
| `pop(out)` | Move the top value into `out` and then remove it. |
| Top/state queries | Inspect the top element, size, empty, full, and capacity. |

## Example

```cpp
castle::container::heap<int, 8> h;
h.emplace(10);
h.emplace(3);
int top = h.top();
h.pop();
```

## Embedded notes

- The comparator controls min-heap vs max-heap behavior.
- Sift operations are O(log N).
