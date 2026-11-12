# basic_cache

## Overview
`castle::cache::basic_cache<Key, T, N, Policy, Hash, KeyEqual>` is the fixed-capacity key/value cache engine shared by every Castle cache policy. It owns in-place storage for `N` entries and a chained hash index built from compact slot indices. The compile-time `Policy` decides which entry to evict when a new key arrives and the cache is full. Most code uses the short aliases [`fifo`](fifo.md), [`lifo`](lifo.md), [`lru`](lru.md), [`mru`](mru.md), and [`lfu`](lfu.md). Spell `basic_cache` directly when you need a custom key hash, equality, LFU counter width, or your own policy.

## Header
`#include "castle/cache/basic_cache.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`../error/status.md`](../error/status.md)
- [`../memory/construct.md`](../memory/construct.md)
- [`../memory/destroy.md`](../memory/destroy.md)
- [`../memory/object.md`](../memory/object.md)
- [`../memory/static_storage.md`](../memory/static_storage.md)
- [`../utility/compare.md`](../utility/compare.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/hash.md`](../utility/hash.md)
- [`../utility/move.md`](../utility/move.md)

## Public API
| API | Description | Complexity |
|---|---|---|
| `basic_cache<Key, T, N, Policy, Hash = castle::hash<Key>, KeyEqual = castle::equal_to<Key>>` | Cache holding at most `N` entries. `N > 0` is enforced by `static_assert`. | - |
| `basic_cache()`, `explicit basic_cache(const Hash&, const KeyEqual& = KeyEqual())` | Empty cache with default or caller-supplied functors. | O(N) |
| `key_type`, `mapped_type`, `size_type`, `policy_type`, `hasher`, `key_equal` | Member type aliases. | - |
| `capacity()`, `size()`, `empty()`, `full()` | Capacity/state queries. `full()` means the next `put()` of a new key evicts. | O(1) |
| `put(key, value)` | Inserts, or updates an existing key in place. Copy and move overloads. Returns `T&` to the cached value. Evicts the policy victim first when full. | O(1) avg |
| `put(key, value, on_evict)` | Same, and calls `on_evict(const Key&, T&)` with the victim just before it is destroyed. Not called on update or when there is room. | O(1) avg |
| `get(key)` | Returns `T*` or `nullptr`. A hit counts as an access. | O(1) avg |
| `peek(key)` (const and non-const), `contains(key)` | Lookup that does **not** count as an access. | O(1) avg |
| `erase(key)` | Removes a key without calling any callback. Returns `status::ok` or `status::not_found`. | O(1) avg |
| `evict()`, `evict(on_evict)` | Removes the current victim. Returns `status::ok` or `status::empty`. Repeated calls drain the cache in eviction order. | O(1) avg |
| `clear()` | Destroys every entry without callbacks and resets policy state. | O(N) |

### Policy concept
A policy is a tag type that exposes `template <size_type N> using state = S;`. `S` must be default-constructible (starting empty) and provide:

| Member | Called when |
|---|---|
| `void reset()` | `clear()` |
| `void on_insert(size_type slot)` | A new key was stored in `slot` (at least one slot was free) |
| `void on_access(size_type slot)` | `get()` hit, or `put()` updated an existing key |
| `void on_erase(size_type slot)` | `erase()`, eviction |
| `size_type victim() const` | Eviction is needed. Called only while the cache is non-empty |

Built-in tags: `fifo_policy`, `lifo_policy`, `lru_policy`, `mru_policy`, `lfu_policy<Count = uint32_t>`.

## Usage Example
See [`samples/cache/basic_cache.cpp`](../../samples/cache/basic_cache.cpp).

```cpp
struct node_id { uint8_t bus; uint8_t address; };
struct node_id_hash { castle::size_type operator()(const node_id& id) const noexcept; };
struct node_id_equal { bool operator()(const node_id& a, const node_id& b) const noexcept; };

castle::cache::basic_cache<node_id, int16_t, 8U, castle::cache::lru_policy, node_id_hash, node_id_equal> temps;
temps.put(node_id{0U, 0x48U}, 215);
if (int16_t* t = temps.get(node_id{0U, 0x48U}))
{
    // use *t
}
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, or virtual dispatch. All storage, including the hash index and policy bookkeeping, is embedded in the cache object.
- Average O(1) for every operation except `clear()` and construction (O(N)). The worst case is O(N), reached only when every key hashes to the same bucket.
- No tombstones: an erased or evicted slot is unlinked and reused straight away, so lookups do not slow down as the cache churns. (`dsa::hash_table` keeps tombstones until `clear()`, which is why the cache does not use it.)
- Pointers and references returned by `put()`, `get()`, and `peek()` stay valid until that entry is erased, evicted, or cleared. Entries never move.
- The `value` passed to `put()` must not refer to a value already in the same cache, because that entry may be evicted before the copy is made. `on_evict` must not modify the cache.
- Non-copyable and non-movable. Not thread-safe: guard a shared instance with `castle::mutex` and `castle::scoped_mutex` (see [`../sync/scoped_mutex.md`](../sync/scoped_mutex.md)).
- `Key` must be copy-constructible and supported by `Hash` (the default `castle::hash<Key>` covers integral, enum, pointer, and string-view keys).
- Slot indices use the smallest unsigned type that can hold `N` (`uint8_t` up to 255, `uint16_t` up to 65535, then `uint32_t`). Fixed overhead beyond the entries is `2·N` indices for the hash index, plus the policy state: `2·N + 2` indices for FIFO/LIFO/LRU/MRU, and `7·N + 2` indices plus `N` counters for LFU.
- Hash buckets: `N`, selected as `hash(key) % N`.
