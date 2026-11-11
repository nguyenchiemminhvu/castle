# list

## Overview
`castle::container::list<T, N>` is a fixed-capacity doubly linked list (deque-like) backed by an internal node pool. It supports O(1) insertion and removal at both ends and at any known position, bidirectional and reverse iteration, and O(1) repositioning of an existing element without copying it. That last property (`move_to_front()` / `move_to_back()` / `splice()`) makes it the natural recency list for an LRU cache. No heap allocation is used; compare with the singly linked [`forward_list`](forward_list.md).

## Header
`#include "castle/container/list.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`initializer_list.md`](initializer_list.md)
- [`../error/status.md`](../error/status.md)
- [`../memory/object.md`](../memory/object.md)
- [`../memory/construct.md`](../memory/construct.md)
- [`../memory/destroy.md`](../memory/destroy.md)
- [`../memory/static_storage.md`](../memory/static_storage.md)
- [`../iterator/tags.md`](../iterator/tags.md)
- [`../iterator/reverse_iterator.md`](../iterator/reverse_iterator.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/move.md`](../utility/move.md)

## Public API
| API | Description | Complexity |
|---|---|---|
| `list<T, N>` | Doubly linked list with room for at most `N` live nodes; `N > 0` is enforced by `static_assert`. | - |
| `list()`, `list(initializer_list<T>)` | Default and brace-initializer constructors; source order is preserved, surplus elements are ignored. | O(N) |
| `capacity()`, `max_size()`, `size()`, `available()`, `empty()`, `full()` | Capacity/state queries. | O(1) |
| `begin()/end()`, `cbegin()/cend()` | Bidirectional iterators. `end()` is decrementable (`--end()` is the last element). | O(1) |
| `rbegin()/rend()`, `crbegin()/crend()` | Reverse iterators (`castle::reverse_iterator`). | O(1) |
| `front()`, `back()` | Access to the first/last element; undefined on an empty list. | O(1) |
| `push_front()`, `emplace_front()`, `push_back()`, `emplace_back()` | End insertion; return `status::ok` or `status::full` when the pool is exhausted. | O(1) |
| `pop_front()`, `pop_back()` | End removal; return `status::ok` or `status::empty`. | O(1) |
| `emplace(pos, args...)`, `insert(pos, value)` | Insert **before** `pos` (`end()` appends). Return an iterator to the new element, or `end()` when the pool is exhausted (a successful insert never returns `end()`). | O(1) |
| `erase(pos)` | Erase one element and return the iterator that followed it. `erase(end())` is a no-op that returns `end()`. | O(1) |
| `erase(first, last)` | Erase `[first, last)` and return `last`. | O(distance) |
| `splice(pos, element)` | Relocate `element` (from this list) to sit before `pos`. Allocation-free; no iterator is invalidated. No-op if `element == end()`, `element == pos`, or it is already directly before `pos`. | O(1) |
| `move_to_front(element)`, `move_to_back(element)` | `splice(begin(), element)` / `splice(end(), element)`; the LRU "touch" operations. | O(1) |
| `remove(value)` | Erase every element equal to `value`; returns the number removed. | O(size) |
| `reverse()` | Reverse the order in place by swapping links. | O(size) |
| `find(value)` | First element equal to `value`, or `end()`. | O(size) |
| `clear()` | Destroy all elements and return every node to the free pool. | O(size) |

Iterators: `iterator` and `const_iterator` are bidirectional; `iterator` converts implicitly to `const_iterator`, and mixed `==`/`!=` comparisons work. Mutating members accept `const_iterator`, so both iterator kinds can be passed.

## Usage Example
See [`samples/container/list.cpp`](../../samples/container/list.cpp).

```cpp
castle::container::list<uint32_t, 4U> recency{10U, 20U, 30U};

// "Touch" key 30: it becomes the most recently used entry.
recency.move_to_front(recency.find(30U)); // 30, 10, 20

// Evict the least recently used entry and insert a new one.
recency.pop_back();                       // 30, 10
recency.push_front(40U);                  // 40, 30, 10
```

### LRU cache sketch
Store `list<key>` (or `list<pair<key, value>>`) as the recency order and a `hash_map<key, list::iterator>` as the index. A lookup hit calls `move_to_front(it)`; an insert into a full cache does `back()` + `pop_back()` first, then `push_front()`. Iterators stay valid across `move_to_front()`, so the index never needs updating on a hit.

## Constraints & Notes
- No dynamic allocation; every node comes from an internal pool of `N` nodes, so `sizeof(list)` grows linearly with `N`. Construction is O(N) because the pool is chained.
- Not copyable and not movable (same as `forward_list`); node storage is owned in place.
- Iterators and references remain valid until their own element is erased or the list is cleared. `splice()`, `move_to_*()`, and `reverse()` never invalidate any iterator.
- All iterator arguments must belong to the same list object; passing an iterator of another list, or dereferencing `end()`, is undefined. Default-constructed iterators are singular.
- Internally the list is circular around a sentinel node: incrementing `end()` wraps to the first element and decrementing `begin()` reaches `end()`; do not rely on this.
- Insertion never fails other than by pool exhaustion (`status::full` / `end()`); nothing throws and no RTTI or virtual functions are used.
- Not thread-safe; guard concurrent access externally.
- Reverse iterators come from `castle::reverse_iterator`, whose `operator->` is only usable with pointer base iterators; use `(*rit).member` with `list::reverse_iterator`.
