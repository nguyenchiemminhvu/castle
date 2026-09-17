# Fixed-capacity ring buffer

## Overview

A FIFO circular buffer backed by a fixed array. When `force_push` is used on a full buffer, the oldest item is discarded.

## Header

```cpp
#include "castle/container/ring_buffer.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `ring_buffer<T, N>` | FIFO storage with capacity `N`. |
| `push()` | Add a value only when space is available. |
| `force_push()` | Add a value and evict the oldest item when full. |
| `pop()` | Remove the oldest value. |
| `pop_bulk()` | Read and remove multiple values into a caller-provided buffer. |
| State queries | Size, capacity, empty/full, front/back and clear operations where provided. |

## Example

```cpp
castle::container::ring_buffer<uint8_t, 64> rx;
rx.force_push(byte);
uint8_t first;
rx.pop(first);
```

## Embedded notes

- Index wrapping uses a power-of-two fast path when `N` is a power of two.
- Storage is inside the buffer object.
