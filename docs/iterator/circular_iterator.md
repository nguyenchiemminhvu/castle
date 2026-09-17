# Circular iterator

## Overview

Iterator that wraps around a bounded sequence instead of stopping permanently at the physical end.

## Header

```cpp
#include "castle/iterator/circular_iterator.h"
```

**Namespace:** `castle::iterator`

## Main API

| API | Purpose |
|---|---|
| `circular_iterator` | Wraps an iterator or pointer and cycles through a fixed range. |
| Increment/decrement | Move and wrap at the configured bounds. |
| Comparison/access | Use iterator-style dereference and comparisons. |

## Example

```cpp
// Useful for circular buffers and repeating fixed sequences.
```

## Embedded notes

- Define the cycle bounds carefully; an incorrectly configured range can create endless iteration.
