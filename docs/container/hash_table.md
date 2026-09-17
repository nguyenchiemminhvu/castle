# hash_table

## Overview
`castle::container::hash_table` is the fixed-capacity open-addressed key/value table that underpins Castle hash containers. Use it when you need deterministic hashed lookup with embedded storage and explicit status-code error handling.

## Header
`#include "castle/container/hash_table.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`initializer_list.md`](initializer_list.md)
- [`../iterator/tags.md`](../iterator/tags.md)
- [`../error/status.md`](../error/status.md)
- [`../memory/object.md`](../memory/object.md)
- [`../memory/construct.md`](../memory/construct.md)
- [`../memory/destroy.md`](../memory/destroy.md)
- [`../memory/static_storage.md`](../memory/static_storage.md)
- [`../utility/compare.md`](../utility/compare.md)
- [`../utility/hash.md`](../utility/hash.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/move.md`](../utility/move.md)
- [`../utility/pair.md`](../utility/pair.md)

## Public API
| API | Description |
|---|---|
| `hash_table<Key, T, N, Hash, KeyEqual>` | Fixed-capacity associative table with exactly `N` probe slots. |
| Constructors | Default, hash-functor, and `initializer_list` constructors. |
| `capacity()`, `size()`, `available()`, `empty()`, `full()` | O(1) capacity/state queries. |
| `begin()/end()`, `cbegin()/cend()` | Forward iterators over occupied slots only. |
| `insert(key, value)` | Insert by copy or move; returns `status::already_exists` or `status::full` on failure. |
| `emplace(key, args...)`, `try_emplace(key, args...)` | Construct a mapped value only when the key is absent. |
| `insert_or_assign(key, value)` | Insert a new entry or overwrite an existing mapped value. |
| `find(key)`, `contains(key)`, `get(key)` | Lookup helpers; `get()` returns a mapped-value pointer or `nullptr`. |
| `erase(key)` | Destroy an entry and leave a tombstone; returns `status::not_found` when absent. |
| `clear()` | O(N) destroy all occupied entries and reset the slot states. |

## Usage Example
See [`samples/sample_hash_table.cpp`](../../samples/sample_hash_table.cpp).

```cpp
castle::container::hash_table<int, int, 8U> table;
table.emplace(7, 70);
table.insert_or_assign(7, 71);
CASTLE_SAMPLE_CHECK(*table.get(7) == 71);
```

## Constraints & Notes
- No dynamic allocation; all entries are stored inside the table object.
- Open addressing uses linear probing with `empty`, `occupied`, and `deleted` slot states.
- Average successful lookup/insertion/erase is typically O(1); worst case is O(N).
- Erasing an entry leaves a tombstone so later probes continue through the collision chain.
- Iterators remain valid across insertion because entries never move; erasing an element invalidates only iterators to that element, and `clear()` invalidates all iterators.
