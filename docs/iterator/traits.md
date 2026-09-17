# Iterator traits

## Overview

Extracts common type information from iterators without relying on `<iterator>`.

## Header

```cpp
#include "castle/iterator/traits.h"
```

**Namespace:** `castle::iterator`

## Main API

| API | Purpose |
|---|---|
| `iterator_traits<It>` | Provides value, difference, pointer, reference, and category information where supported. |
| Specializations | Pointer iterators and Castle iterator types are handled by the trait machinery. |

## Example

```cpp
using value_t = castle::iterator::iterator_traits<int*>::value_type;
```

## Embedded notes

- Use these traits in generic code that must stay STL-independent.
