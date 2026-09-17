# Fixed-range iterator

## Overview

Iterator helper for walking a fixed range represented by pointer/index information.

## Header

```cpp
#include "castle/iterator/fixed_iterator.h"
```

**Namespace:** `castle::iterator`

## Main API

| API | Purpose |
|---|---|
| `fixed_iterator` | Iterator over a bounded fixed sequence. |
| Increment/decrement | Move inside the configured range. |
| Dereference/comparison | Use normal iterator-style access. |

## Example

```cpp
// Used internally by fixed-capacity structures where a stable range is known.
```

## Embedded notes

- The iterator does not allocate or own the underlying storage.
