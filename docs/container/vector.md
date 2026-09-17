# Fixed-capacity vector

## Overview

A vector-like sequence with compile-time capacity. Live objects occupy the first `size()` slots of internal storage.

## Header

```cpp
#include "castle/container/vector.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `vector<T, N>` | Contiguous sequence with maximum size `N`. |
| `size()` / `capacity()` / `empty()` / `full()` | Container state. |
| Iterators | Forward and reverse iterators, including const forms. |
| Element access | `operator[]`, `front`, `back`, `data`. |
| `emplace_back()` / `push_back()` | Construct or add an element if capacity remains. |
| `pop_back()` / `clear()` | Destroy elements and reduce size. |

## Example

```cpp
castle::container::vector<int, 4> v;
v.emplace_back(10);
v.push_back(20);
v.pop_back();
```

## Embedded notes

- No capacity growth occurs; a full insertion returns `status::full`.
- Destruction is explicit for live elements, so non-trivial embedded types are supported.
