# forward_list

## Overview
`castle::container::forward_list` is a fixed-capacity singly linked list backed by an internal node pool. Use it when you want stable node addresses and O(1) insertion or erasure after a known position without heap allocation.

## Header
`#include "castle/container/forward_list.hpp"`

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
- [`../iterator/tags.md`](../iterator/tags.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/move.md`](../utility/move.md)

## Public API
| API | Description |
|---|---|
| `forward_list<T, N>` | Singly linked list with room for at most `N` live nodes. |
| Constructors | Default and `initializer_list` constructors; list-order is preserved. |
| `capacity()`, `max_size()`, `size()`, `available()`, `empty()`, `full()` | O(1) capacity/state queries. |
| `begin()/end()`, `cbegin()/cend()`, `before_begin()` | Forward iterators plus a pre-head sentinel iterator. |
| `front()` | O(1) access to the head element; undefined on an empty list. |
| `push_front()`, `emplace_front()` | O(1) front insertion; return `status::full` when the pool is exhausted. |
| `pop_front()` | O(1) front removal; returns `status::empty` when the list is empty. |
| `emplace_after(pos, args...)` | O(1) insertion after `pos`; returns `status::out_of_range` for `end()` and `status::full` when no node is free. |
| `erase_after(pos)` | O(1) erase-after operation; returns the iterator that followed the erased node. |
| `remove(value)` | O(size) erase-all-equal-values operation. |
| `reverse()` | O(size) in-place reversal. |
| `find(value)` | O(size) linear search. |
| `clear()` | O(size) destroy all live nodes and return them to the free pool. |

## Usage Example
See [`samples/sample_forward_list.cpp`](../../samples/sample_forward_list.cpp).

```cpp
castle::container::forward_list<int, 4U> values{1, 2};
values.emplace_after(values.before_begin(), 0);
CASTLE_SAMPLE_CHECK(values.front() == 0);
```

## Constraints & Notes
- No dynamic allocation; every node comes from an internal pool of `N` nodes.
- `push_front`, `emplace_front`, `emplace_after`, `pop_front`, and `erase_after` are O(1); search, `remove`, `reverse`, and `clear` are O(size).
- Iterators and references remain valid until the pointed-to node is erased or the list is cleared.
- `front()` is undefined on an empty list.
- Insertions fail with `status::full` when no free nodes remain.
