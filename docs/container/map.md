# map

## Overview
Fixed-capacity ordered map backed by an AVL tree. Use it when key/value pairs must stay sorted by key and the maximum entry count is known at compile time.

## Header
`#include "castle/container/map.hpp"`

## Dependencies
- [avl_tree](avl_tree.md)
- [initializer_list](initializer_list.md)
- [error_handler](../core/error_handler.md)

## Public API
| API | Description |
| --- | --- |
| `map<Key, T, N, Compare>` | Ordered key/value container with compile-time capacity `N`. |
| Constructors | Default constructor, comparator constructor, and initializer-list constructor. The initializer-list constructor asserts `list.size() <= N` and ignores duplicate keys. |
| `capacity()`, `max_size()`, `size()`, `available()`, `empty()`, `full()` | Capacity/state queries. O(1). |
| `begin()/end()/cbegin()/cend()` | Sorted iterators over `pair<const Key, T>`. `begin()` is O(log N) worst-case; `end()` is O(1). |
| `insert(value)`, `insert(key, value)` | Copy or move insertion helpers. Return `status::ok`, `status::already_exists`, or `status::full`. O(log N) worst-case. |
| `try_emplace(key, ...)` | In-place mapped-value construction. Returns `status::ok`, `status::already_exists`, or `status::full`. O(log N) worst-case. |
| `find(key)`, `contains(key)`, `get(key)` | Lookup helpers. `find()` returns `end()` on miss; `get()` returns `nullptr`. O(log N) worst-case. |
| `lower_bound(key)`, `upper_bound(key)` | Ordered bound queries. O(log N) worst-case. |
| `erase(key)` | Removes by key. Returns `status::ok` or `status::not_found`. O(log N) worst-case. |
| `erase(iterator)` | Removes by iterator and returns the next iterator. O(log N) worst-case. |
| `clear()` | Removes all entries. O(N). |

## Usage Example
See `samples/sample_map.cpp`.

```cpp
castle::container::map<int, int, 3U> values;
values.insert(2, 20);
values.try_emplace(1, 10);
const int* mapped = values.get(2);
```

## Constraints & Notes
- No dynamic allocation; nodes live in fixed internal storage.
- Iteration order is always sorted by `Compare`.
- Iterators and references to non-erased entries remain valid across insertions and unrelated erasures; `clear()` invalidates everything.
- Not thread-safe; concurrent access requires external synchronization.
