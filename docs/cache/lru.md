# LRU cache

## Overview
`castle::cache::lru<Key, T, N>` is a fixed-capacity Least Recently Used cache. Every `get()` hit and every `put()` (insert or update) marks the entry as most recently used. When full, it evicts the entry that has gone **longest without any access**. This exploits temporal locality and is the usual default for sector, page, and lookup caches.

## Header
`#include "castle/cache/lru.hpp"`

## Dependencies
- [`basic_cache.md`](basic_cache.md)

## Public API
| API | Description | Complexity |
|---|---|---|
| `lru_policy` | Policy tag. Victim: the least recently used entry. `get()` hits and every `put()` refresh an entry. `peek()` and `contains()` do not. | - |
| `lru<Key, T, N, Hash = castle::hash<Key>, KeyEqual = castle::equal_to<Key>>` | Alias of `basic_cache<Key, T, N, lru_policy, Hash, KeyEqual>`. | - |
| `capacity()`, `size()`, `empty()`, `full()` | Capacity/state queries. `full()` means the next `put()` of a new key evicts. | O(1) |
| `put(key, value)` | Inserts, or updates an existing key in place. Copy and move overloads. Returns `T&` to the cached value. Evicts the policy victim first when full. | O(1) avg |
| `put(key, value, on_evict)` | Same, and calls `on_evict(const Key&, T&)` with the victim just before it is destroyed. Not called on update or when there is room. | O(1) avg |
| `get(key)` | Returns `T*` or `nullptr`. A hit counts as an access. | O(1) avg |
| `peek(key)` (const and non-const), `contains(key)` | Lookup that does **not** count as an access. | O(1) avg |
| `erase(key)` | Removes a key without calling any callback. Returns `status::ok` or `status::not_found`. | O(1) avg |
| `evict()`, `evict(on_evict)` | Removes the current victim. Returns `status::ok` or `status::empty`. Repeated calls drain the cache in eviction order. | O(1) avg |
| `clear()` | Destroys every entry without callbacks and resets policy state. | O(N) |

## Usage Example
See [`samples/cache/lru.cpp`](../../samples/cache/lru.cpp).

```cpp
castle::cache::lru<uint32_t, sector, 2U> cache;
auto write_back = [](const uint32_t& addr, sector& s)
{
    if (s.dirty)
    {
        flash_write(addr, s);
    }
};
cache.put(0x0000U, sector{}, write_back);
cache.put(0x1000U, sector{}, write_back);
cache.get(0x0000U);                        // 0x0000 becomes most recently used
cache.put(0x2000U, sector{}, write_back);  // evicts 0x1000
while (cache.evict(write_back) == castle::status::ok)  // flush all
{
}
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
