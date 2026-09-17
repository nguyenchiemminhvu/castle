# set

## Overview
Fixed-capacity ordered set backed by an AVL tree. Use it when unique keys must stay sorted and the maximum node count is fixed at compile time.

## Header
`#include "castle/container/set.hpp"`

## Dependencies
- [avl_tree](avl_tree.md)
- [initializer_list](initializer_list.md)
- [error_handler](../core/error_handler.md)

## Public API
| API | Description |
| --- | --- |
| `set<Key, N, Compare>` | Ordered set with compile-time capacity `N`. |
| `const_iterator` / `iterator` | Bidirectional iterator over keys in sorted order. `iterator` is an alias of `const_iterator`; keys are immutable. |
| Constructors | Default constructor, comparator constructor, and initializer-list constructor. The initializer-list constructor asserts `list.size() <= N` and ignores duplicates. |
| `capacity()`, `max_size()`, `size()`, `available()`, `empty()`, `full()` | Capacity/state queries. O(1). |
| `begin()`, `end()`, `cbegin()`, `cend()` | Sorted iteration. `begin()` is O(log N) worst-case; `end()` is O(1). |
| `insert(key)` | Inserts a key. Returns `status::ok`, `status::already_exists`, or `status::full`. O(log N) worst-case. |
| `find(key)`, `contains(key)` | Lookup helpers. `find()` returns `end()` on miss. O(log N) worst-case. |
| `lower_bound(key)`, `upper_bound(key)` | Ordered bound queries. O(log N) worst-case. |
| `erase(key)` | Removes a key. Returns `status::ok` or `status::not_found`. O(log N) worst-case. |
| `clear()` | Removes all keys. O(N). |

## Usage Example
See `samples/sample_set.cpp`.

```cpp
castle::container::set<int, 4U> ids{4, 2};
ids.insert(3);
auto it = ids.lower_bound(3);
```

## Constraints & Notes
- No dynamic allocation; nodes live in fixed internal storage.
- Iteration order is always sorted by `Compare`.
- Iterators and references to non-erased elements remain valid across insertions and unrelated erasures; `clear()` invalidates everything.
- Not thread-safe; concurrent access requires external synchronization.
