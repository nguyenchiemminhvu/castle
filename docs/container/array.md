# Fixed-size array

## Overview

A fixed-size contiguous container. All elements are stored inside the array object; there is no heap allocation.

## Header

```cpp
#include "castle/container/array.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `array<T, N>` | Fixed-size container with `N` elements. |
| `size()` / `capacity()` / `empty()` / `full()` | Compile-time and runtime size information. |
| Iterators | `begin/end`, const and reverse iterators. |
| Element access | `operator[]`, `front()`, `back()`, `data()`. |
| `static_size` | Compile-time element count. |

## Example

```cpp
castle::container::array<int, 3> a{1, 2, 3};
a[1] = 20;
for (auto it = a.begin(); it != a.end(); ++it) { /* ... */ }
```

## Embedded notes

- `array<T, 0>` is a valid zero-sized specialization.
- Use this when the size never changes.
