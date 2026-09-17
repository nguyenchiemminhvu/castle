# basic_heap

## Overview
`castle::container::basic_heap` is a fixed-capacity binary heap stored entirely in internal memory. Use it when you need bounded priority-queue behavior with deterministic O(log N) insertion and removal.

## Header
`#include "castle/container/heap.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`initializer_list.md`](initializer_list.md)
- [`../error/status.md`](../error/status.md)
- [`../memory/object.md`](../memory/object.md)
- [`../memory/construct.md`](../memory/construct.md)
- [`../memory/destroy.md`](../memory/destroy.md)
- [`../memory/static_storage.md`](../memory/static_storage.md)
- [`../utility/compare.md`](../utility/compare.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/move.md`](../utility/move.md)

## Public API
| API | Description |
|---|---|
| `basic_heap<T, N, Compare>` | Fixed-capacity heap with `Compare` deciding which element has higher priority. |
| `max_heap<T, N, Compare>` / `min_heap<T, N, Compare>` | Convenience aliases for greatest-first and smallest-first heaps. |
| Constructors | Default, comparator-taking, and `initializer_list` constructors. |
| `capacity()`, `size()`, `available()`, `empty()`, `full()` | O(1) capacity/state queries. |
| `top()` | O(1) access to the root element; undefined on an empty heap. |
| `push(const T&)`, `push(T&&)`, `emplace(args...)` | O(log N) insertion; return `status::ok` or `status::full`. |
| `pop()` / `pop(T&)` | O(log N) remove-root operations; return `status::ok` or `status::empty`. |
| `remove_at(index)` | O(log N) erase by internal heap-array index; returns `status::out_of_range` for an invalid index. |
| `replace_top(T&)`, `replace_top(const T&)` | O(log N) replace the root and re-sift; return `status::empty` if the heap is empty. |
| `begin()/end()` | Iterators over the internal heap-array order, not sorted order. |
| `clear()` | O(size) destroy all stored elements. |

## Usage Example
See [`samples/sample_heap.cpp`](../../samples/sample_heap.cpp).

```cpp
castle::container::min_heap<int, 6U> queue;
queue.push(30);
queue.emplace(10);
queue.push(20);
CASTLE_SAMPLE_CHECK(queue.top() == 10);
```

## Constraints & Notes
- No dynamic allocation; storage for `N` elements is embedded in the heap.
- `push`, `emplace`, `pop`, `remove_at`, and `replace_top` are O(log N); `top` and size queries are O(1).
- Iteration exposes internal heap layout, not priority order.
- A full insertion returns `status::full`; popping or replacing on an empty heap returns `status::empty`.
- The heap is non-copyable and non-movable.
