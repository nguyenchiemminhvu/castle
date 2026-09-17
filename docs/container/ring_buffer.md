# ring_buffer

## Overview
Fixed-capacity FIFO circular buffer with inline storage. Use it for deterministic queueing where overwrite-on-full should be explicit and controlled.

## Header
`#include "castle/container/ring_buffer.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)
- [error_handler](../core/error_handler.md)
- [array](array.md)
- [initializer_list](initializer_list.md)
- [move](../utility/move.md)

## Public API
| API | Description |
| --- | --- |
| `ring_buffer<T, N>` | FIFO buffer with compile-time capacity `N`. Requires `T` to be trivially copyable and trivially destructible; `N > 0`. |
| Constructors | Default constructor and initializer-list constructor. The initializer-list constructor asserts `list.size() <= N` and pushes in list order. |
| `capacity()`, `size()`, `available()`, `empty()`, `full()` | Capacity/state queries. O(1). |
| `clear()` | Removes all elements. O(1). |
| `push(value)` | Appends when space is available. Returns `true` on success, `false` when full. O(1). |
| `force_push(value)` | Appends and evicts the oldest element when full. Returns `true` when eviction occurred. O(1). |
| `pop(out)`, `pop()` | Removes the oldest element with or without copying it out. Returns `false` when empty. O(1). |
| `peek(index, out)` | Copies the element at logical FIFO position `index`. Returns `false` when out of range. O(1). |
| `front()`, `back()` | Oldest/newest element access. O(1); buffer must be non-empty. |
| `push_bulk(src, max)` | Appends up to `max` values. Returns count written. O(min(max, available())). |
| `pop_bulk(dst, max)` | Removes up to `max` oldest values. Returns count read. O(min(max, size())). |

## Usage Example
See `samples/sample_ring_buffer.cpp`.

```cpp
castle::container::ring_buffer<uint8_t, 4U> fifo{1U, 2U};
fifo.push(3U);
fifo.force_push(4U);
```

## Constraints & Notes
- No dynamic allocation; storage is an internal `array<T, N>`.
- `force_push()` invalidates references to an evicted front element.
- The container has no iterators; references from `front()`/`back()` remain valid only until a mutating operation changes that slot.
- Not thread-safe; concurrent access requires external synchronization.
