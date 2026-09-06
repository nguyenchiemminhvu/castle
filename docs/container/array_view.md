# Non-owning array view

## Overview

A lightweight view over contiguous elements. It does not own the memory and never allocates.

## Header

```cpp
#include "castle/container/array_view.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `array_view<T>` | Pointer plus length view. |
| Constructors | Build from `(pointer, size)` or a compatible container with `data()` and `size()`. |
| `size()` / `empty()` | View state. |
| Element access and iterators | `operator[]`, `front`, `back`, `data`, `begin`, `end`. |
| `subview(offset, count)` | Create a bounded child view. |
| `make_array_view()` | Convenience factory overloads. |

## Example

```cpp
castle::container::array<int, 4> a{1,2,3,4};
auto view = castle::container::make_array_view(a.data(), a.size());
auto tail = view.subview(2, 2);
```

## Embedded notes

- The referenced storage must outlive the view.
- No ownership or capacity is tracked.
