# LIFO cache

## Overview
`castle::cache::lifo<Key, T, N>` is a fixed-capacity Last-In, First-Out cache that behaves like a bounded stack. When full, it evicts the **most recently inserted** entry, so the earliest entries stay resident. Reads and updates never change an entry's position. Use it to keep a stable base set, such as boot-time parameters, while transient entries rotate on top.

## Header
`#include "castle/cache/lifo.hpp"`

## Dependencies
- [`basic_cache.md`](basic_cache.md)

## Public API
| API | Description | Complexity |
|---|---|---|
| `lifo_policy` | Policy tag. Victim: the most recently inserted entry. `get()` and an updating `put()` do **not** change an entry's position. | - |
| `lifo<Key, T, N, Hash = castle::hash<Key>, KeyEqual = castle::equal_to<Key>>` | Alias of `basic_cache<Key, T, N, lifo_policy, Hash, KeyEqual>`. | - |
| `capacity()`, `size()`, `empty()`, `full()` | Capacity/state queries. `full()` means the next `put()` of a new key evicts. | O(1) |
| `put(key, value)` | Inserts, or updates an existing key in place. Copy and move overloads. Returns `T&` to the cached value. Evicts the policy victim first when full. | O(1) avg |
| `put(key, value, on_evict)` | Same, and calls `on_evict(const Key&, T&)` with the victim just before it is destroyed. Not called on update or when there is room. | O(1) avg |
| `get(key)` | Returns `T*` or `nullptr`. A hit counts as an access. | O(1) avg |
| `peek(key)` (const and non-const), `contains(key)` | Lookup that does **not** count as an access. | O(1) avg |
| `erase(key)` | Removes a key without calling any callback. Returns `status::ok` or `status::not_found`. | O(1) avg |
| `evict()`, `evict(on_evict)` | Removes the current victim. Returns `status::ok` or `status::empty`. Repeated calls drain the cache in eviction order. | O(1) avg |
| `clear()` | Destroys every entry without callbacks and resets policy state. | O(N) |

## Usage Example
See [`samples/cache/lifo.cpp`](../../samples/cache/lifo.cpp).

```cpp
castle::cache::lifo<uint16_t, int32_t, 3U> params;
params.put(1U, 115200);
params.put(2U, 8);
params.put(50U, -3);
params.put(51U, 7);    // evicts 50, the most recently inserted
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, or virtual dispatch. All storage, including the hash index and policy bookkeeping, is embedded in the cache object.
- Average O(1) for every operation except `clear()` and construction (O(N)). The worst case is O(N), reached only when every key hashes to the same bucket.
- No tombstones: an erased or evicted slot is unlinked and reused straight away, so lookups do not slow down as the cache churns. (`container::hash_table` keeps tombstones until `clear()`, which is why the cache does not use it.)
- Pointers and references returned by `put()`, `get()`, and `peek()` stay valid until that entry is erased, evicted, or cleared. Entries never move.
- The `value` passed to `put()` must not refer to a value already in the same cache, because that entry may be evicted before the copy is made. `on_evict` must not modify the cache.
- Non-copyable and non-movable. Not thread-safe: guard a shared instance with `castle::mutex` and `castle::scoped_mutex` (see [`../sync/scoped_mutex.md`](../sync/scoped_mutex.md)).
- `Key` must be copy-constructible and supported by `Hash` (the default `castle::hash<Key>` covers integral, enum, pointer, and string-view keys).
- Policy state: two slot-index links per entry plus head/tail (`2·N + 2` indices). Every policy hook is O(1).
