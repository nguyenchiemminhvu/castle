# hash_map

## Overview
Fixed-capacity hash map alias built on Castle's deterministic open-addressed hash table. Use it for key/value lookup when the maximum entry count is known at compile time and heap allocation is forbidden.

## Header
`#include "castle/container/hash_map.hpp"`

## Dependencies
- [hash_table](hash_table.md)

## Public API
| API | Description |
| --- | --- |
| `hash_map<Key, T, N, Hash, KeyEqual>` | Alias of `hash_table<Key, T, N, Hash, KeyEqual>`. Capacity is fixed at `N`. |
| Constructors | Default constructor, hash-functor constructor, and initializer-list constructor inherited from `hash_table`. |
| `capacity()`, `size()`, `available()`, `empty()`, `full()` | Capacity/state queries. `capacity()` is compile-time fixed; others are O(1). |
| `begin()`, `end()`, `cbegin()`, `cend()` | Iterate occupied slots in slot order. `begin()` is O(N) worst-case; `end()` is O(1). |
| `insert(key, value)`, `emplace(key, ...)`, `try_emplace(key, ...)` | Insert when the key is absent. Return `status::ok`, `status::already_exists`, or `status::full`. O(N) worst-case. |
| `insert_or_assign(key, value)` | Inserts or overwrites the mapped value. Returns `status::ok` or `status::full`. O(N) worst-case. |
| `find(key)`, `contains(key)`, `get(key)` | Lookup helpers. `find()` returns `end()` on miss; `get()` returns `nullptr`. O(N) worst-case. |
| `erase(key)` | Removes a key. Returns `status::ok` or `status::not_found`. O(N) worst-case. |
| `clear()` | Removes all entries. O(N). |

## Usage Example
See `samples/sample_hash_map.cpp`.

```cpp
castle::container::hash_map<int, int, 3U> values;
values.insert(1, 10);
values.try_emplace(2, 20);
values.insert_or_assign(1, 15);
```

## Constraints & Notes
- No dynamic allocation, exceptions, RTTI, or virtual dispatch.
- Linear probing means insert/find/erase are O(N) worst-case.
- Iterators and references to erased entries are invalidated by `erase()`. Other entries do not move; `clear()` invalidates everything.
- Not thread-safe; concurrent access requires external synchronization.
