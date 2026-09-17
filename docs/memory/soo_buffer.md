# Small object in-place buffer

## Overview

A small fixed buffer intended to store one object without heap allocation.

## Header

```cpp
#include "castle/memory/soo_buffer.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| `soo_buffer` | Compile-time size/alignment storage for one callable or object. |
| Address/access | Expose storage suitable for placement construction. |

## Example

```cpp
// Use an appropriately sized soo_buffer for a small embedded object.
```

## Embedded notes

- The contained object must fit the configured size and alignment.
