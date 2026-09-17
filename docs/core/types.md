# Castle fundamental types

## Overview

Defines small common type aliases shared by Castle headers, including the library size and difference types.

## Header

```cpp
#include "castle/core/types.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `castle::size_type` | Unsigned size/index type used by containers and algorithms. |
| `castle::difference_type` | Signed difference type used by iterators and ranges. |

## Example

```cpp
castle::size_type count = 8;
castle::difference_type delta = -1;
```

## Embedded notes

- Centralizing these types helps keep the library consistent across target ABIs.
