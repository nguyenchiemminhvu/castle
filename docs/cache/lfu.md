# LFU cache

## Overview
`castle::cache::lfu<Key, T, N>` is a fixed-capacity Least Frequently Used cache. Each entry has an access count: insertion sets it to 1, and every `get()` hit or updating `put()` adds 1. When full, the cache evicts the entry with the **lowest count**. Ties within the lowest count are broken by **LRU order**: the least recently used entry in that bucket goes first, so eviction is fully deterministic. Use LFU when long-term popularity predicts reuse better than recency, for example glyph, font, or configuration caches.

## Header
`#include "castle/cache/lfu.hpp"`

## Dependencies
- [`basic_cache.md`](basic_cache.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/type_ranges.md`](../core/type_ranges.md)

## Public API
| API | Description | Complexity |
|---|---|---|
| `lfu_policy<Count = uint32_t>` | Policy tag. `Count` must be an unsigned integral type. Counts saturate at `numeric_limits<Count>::max()`. A saturated entry still refreshes its recency. `peek()` and `contains()` do not count. | - |
| `lfu<Key, T, N, Hash = castle::hash<Key>, KeyEqual = castle::equal_to<Key>>` | Alias of `basic_cache<Key, T, N, lfu_policy<>, Hash, KeyEqual>`. | - |
| `basic_cache<Key, T, N, lfu_policy<uint8_t>>` | Spelling for a narrower counter. Saves RAM when saturation at 255 is acceptable. | - |
| `capacity()`, `size()`, `empty()`, `full()` | Capacity/state queries. `full()` means the next `put()` of a new key evicts. | O(1) |
| `put(key, value)` | Inserts, or updates an existing key in place. Copy and move overloads. Returns `T&` to the cached value. Evicts the policy victim first when full. | O(1) avg |
| `put(key, value, on_evict)` | Same, and calls `on_evict(const Key&, T&)` with the victim just before it is destroyed. Not called on update or when there is room. | O(1) avg |
| `get(key)` | Returns `T*` or `nullptr`. A hit counts as an access. | O(1) avg |
| `peek(key)` (const and non-const), `contains(key)` | Lookup that does **not** count as an access. | O(1) avg |
| `erase(key)` | Removes a key without calling any callback. Returns `status::ok` or `status::not_found`. | O(1) avg |
| `evict()`, `evict(on_evict)` | Removes the current victim. Returns `status::ok` or `status::empty`. Repeated calls drain the cache in eviction order. | O(1) avg |
| `clear()` | Destroys every entry without callbacks and resets policy state. | O(N) |

## Usage Example
See [`samples/cache/lfu.cpp`](../../samples/cache/lfu.cpp).

```cpp
castle::cache::lfu<char, uint16_t, 3U> glyphs;
glyphs.put('0', 100U);
glyphs.put('1', 101U);
glyphs.put('A', 200U);
glyphs.get('0');
glyphs.get('1');
glyphs.put('B', 201U);   // evicts 'A' (count 1)
glyphs.put('C', 202U);   // evicts 'B' (newcomer, count 1)
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, or virtual dispatch. All storage, including the hash index and policy bookkeeping, is embedded in the cache object.
- Average O(1) for every operation except `clear()` and construction (O(N)). The worst case is O(N), reached only when every key hashes to the same bucket.
- No tombstones: an erased or evicted slot is unlinked and reused straight away, so lookups do not slow down as the cache churns. (`container::hash_table` keeps tombstones until `clear()`, which is why the cache does not use it.)
- Pointers and references returned by `put()`, `get()`, and `peek()` stay valid until that entry is erased, evicted, or cleared. Entries never move.
- The `value` passed to `put()` must not refer to a value already in the same cache, because that entry may be evicted before the copy is made. `on_evict` must not modify the cache.
- Non-copyable and non-movable. Not thread-safe: guard a shared instance with `castle::mutex` and `castle::scoped_mutex` (see [`../sync/scoped_mutex.md`](../sync/scoped_mutex.md)).
- `Key` must be copy-constructible and supported by `Hash` (the default `castle::hash<Key>` covers integral, enum, pointer, and string-view keys).
- Constant-time LFU: entries sit in frequency buckets kept in ascending count order, and each bucket keeps its entries in recency order. An access moves an entry only to the adjacent bucket, which is reused, created, or bumped in place when the entry is alone. Every policy hook is O(1).
- Policy state: `7·N + 2` slot indices plus `N` counters. At most `N` frequency buckets can exist, so the bucket pool cannot run out.
- New entries start at count 1, so a fresh key is the first eviction candidate unless it is accessed again. This is inherent to pure LFU.
