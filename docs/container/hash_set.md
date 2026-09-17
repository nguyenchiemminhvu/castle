# hash_set

## Overview
Fixed-capacity hash set for unique keys. Use it when membership checks should be hash-based, the live set size is bounded at compile time, and no heap allocation is allowed.

## Header
`#include "castle/container/hash_set.hpp"`

## Dependencies
- [hash_table](hash_table.md)
- [initializer_list](initializer_list.md)
- [error_handler](../core/error_handler.md)

## Public API
| API | Description |
| --- | --- |
| `hash_set<Key, N, Hash, KeyEqual>` | Hash-based set with compile-time capacity `N`. |
| `const_iterator` / `iterator` | Forward iterator over keys. `iterator` is an alias of `const_iterator`; keys are immutable. |
| Constructors | Default constructor, hash-functor constructor, and initializer-list constructor. The initializer-list constructor asserts `list.size() <= N` and ignores duplicate keys. |
| `capacity()`, `size()`, `available()`, `empty()`, `full()` | Capacity/state queries. O(1). |
| `begin()`, `end()`, `cbegin()`, `cend()` | Iterate occupied slots in slot order. `begin()` is O(N) worst-case; `end()` is O(1). |
| `insert(key)` | Inserts a key. Returns `status::ok`, `status::already_exists`, or `status::full`. O(N) worst-case. |
| `contains(key)` | Membership check. O(N) worst-case. |
| `find(key)` | Returns an iterator to the key or `end()`. O(N) worst-case. |
| `erase(key)` | Removes a key. Returns `status::ok` or `status::not_found`. O(N) worst-case. |
| `clear()` | Removes all keys. O(N). |

## Usage Example
See `samples/sample_hash_set.cpp`.

```cpp
castle::container::hash_set<int, 3U> ids{1, 2};
ids.insert(3);
bool present = ids.contains(2);
```

## Constraints & Notes
- No dynamic allocation, exceptions, RTTI, or virtual dispatch.
- Linear probing makes insert/find/erase O(N) worst-case.
- Iterators and references to erased keys are invalidated by `erase()`. Other keys do not move; `clear()` invalidates everything.
- Not thread-safe; concurrent access requires external synchronization.
