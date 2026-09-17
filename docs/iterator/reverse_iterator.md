# Reverse iterator

## Overview

Iterator adaptor that walks a sequence from the end toward the beginning.

## Header

```cpp
#include "castle/iterator/reverse_iterator.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `reverse_iterator<Iterator>` | Reverse adaptor around an existing iterator. |
| Dereference | Access the element before the wrapped base iterator. |
| Movement/comparison | Increment/decrement in reverse order and compare iterators. |
| `base()` | Recover the underlying iterator. |

## Example

```cpp
castle::reverse_iterator<int*> rit(end_ptr);
int x = *rit;
```

## Embedded notes

- The type is used by Castle fixed containers for `rbegin()` and `rend()`.
