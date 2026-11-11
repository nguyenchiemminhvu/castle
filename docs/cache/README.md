# Cache module

## Overview
The `castle/cache` module provides fixed-capacity key/value caches with a compile-time chosen eviction policy. The `basic_cache` engine owns all entry storage and a chained hash index; each policy is a small tag that decides which entry leaves when a new key arrives in a full cache. No heap, no exceptions, no RTTI, and no STL are used.

Choose a policy by access pattern:

| Policy | Use when | Evicts when full |
|---|---|---|
| FIFO | Data is time-ordered and access does not matter | Earliest inserted |
| LIFO | Early entries must stay resident; transient entries rotate above them | Most recently inserted |
| LRU | Recently used entries are likely to be used again | Least recently used |
| MRU | Repeated sequential scans larger than the cache | Most recently used |
| LFU | Long-term popularity predicts future use | Lowest access count, LRU among ties |

## Header
`#include "castle/cache/cache.hpp"` (umbrella), or include a single header:

| Header | Provides |
|---|---|
| [basic_cache.hpp](../../../include/castle/cache/basic_cache.hpp) | `basic_cache<Key, T, N, Policy, Hash, KeyEqual>` engine and the internal `linked_order` and `frequency_order` state |
| [fifo.hpp](../../../include/castle/cache/fifo.hpp) | `fifo_policy`, `fifo<Key, T, N>` |
| [lifo.hpp](../../../include/castle/cache/lifo.hpp) | `lifo_policy`, `lifo<Key, T, N>` |
| [lru.hpp](../../../include/castle/cache/lru.hpp) | `lru_policy`, `lru<Key, T, N>` |
| [mru.hpp](../../../include/castle/cache/mru.hpp) | `mru_policy`, `mru<Key, T, N>` |
| [lfu.hpp](../../../include/castle/cache/lfu.hpp) | `lfu_policy<Count>`, `lfu<Key, T, N>` |
| [cache.hpp](../../../include/castle/cache/cache.hpp) | Umbrella include for all of the above |

Namespace: `castle::cache`. All aliases default `Hash` to `castle::hash<Key>` and `KeyEqual` to `castle::equal_to<Key>`.

## Main API
Every alias shares the `basic_cache` interface. Status codes come from `castle/error/status.hpp`.

| Member | Behavior | Returns |
|---|---|---|
| `put(key, value)` | Copy or move (`T&&`) insert, or update an existing key (counts as an access) | `T&` to the cached value |
| `put(key, value, on_evict)` | Same, and calls `on_evict(const Key&, T&)` on the victim before destruction | `T&` |
| `get(key)` | Lookup; records an access with the policy | `T*` or `nullptr` on miss |
| `peek(key)` | Lookup without changing eviction order | `T*` or `nullptr` on miss |
| `contains(key)` | Membership test without changing eviction order | `bool` |
| `erase(key)` | Remove without eviction callback | `status::ok` or `status::not_found` |
| `evict()` / `evict(on_evict)` | Remove the policy's next victim; repeated calls drain in eviction order | `status::ok` or `status::empty` |
| `clear()` | Destroy all entries and reset policy state | `void` |
| `size()`, `capacity()`, `empty()`, `full()` | Occupancy queries; `capacity()` is `N` | `size_type` / `bool` |

Access rules by policy:

| Policy | `get()` hit | Updating `put()` | Insert |
|---|---|---|---|
| FIFO | No change | No change | Appended as newest |
| LIFO | No change | No change | Appended as newest |
| LRU | Moves to most recent | Moves to most recent | Appended as most recent |
| MRU | Moves to most recent | Moves to most recent | Appended as most recent |
| LFU | Count + 1, LRU position within new count | Count + 1 | Count = 1 |

`peek()` and `contains()` never change order or count under any policy.

## Usage Example
```cpp
#include "castle/cache/cache.hpp"

castle::cache::lru<uint8_t, int32_t, 2U> recent;
recent.put(1U, 10);
recent.put(2U, 20);
recent.get(1U);     // key 1 becomes most recently used
recent.put(3U, 30); // evicts key 2

castle::cache::lfu<uint8_t, int32_t, 2U> popular;
popular.put(1U, 10);
popular.put(2U, 20);
popular.get(1U);    // count(1) = 2, count(2) = 1
popular.put(3U, 30, [](const uint8_t&, int32_t&) {
    // Receives key 2 before it is destroyed.
});
```

A custom counter width for LFU:

```cpp
castle::cache::basic_cache<uint16_t, int32_t, 64U, castle::cache::lfu_policy<uint16_t>> small_counts;
```

Per-header reference pages: [basic_cache](../../cache/basic_cache.md), [fifo](../../cache/fifo.md), [lifo](../../cache/lifo.md), [lru](../../cache/lru.md), [mru](../../cache/mru.md), [lfu](../../cache/lfu.md), [cache](../../cache/cache.md).

## Embedded Notes
- **Memory:** capacity `N` is fixed at compile time. Storage is `static_storage` inside the object, plus `N`-sized index arrays for buckets, chains, and policy links. The index type is the smallest unsigned type that holds `0..N`: `uint8_t` up to `N = 255`, then `uint16_t`, then `uint32_t`.
- **Eviction:** `put()` of a new key into a full cache always evicts exactly one entry. Eviction callbacks run before the victim is destroyed and must not modify the cache.
- **Pointer lifetime:** entries never move while cached. Pointers from `get()`, `peek()`, and `put()` stay valid until that entry is erased, evicted, or the cache is cleared or destroyed.
- **Aliasing:** a `value` argument must not refer to a value stored in the same cache, because that entry may be evicted before the copy is made.
- **Complexity:** lookup, insert, erase, and evict are O(1) on average. Worst case is O(N) when all keys hash to one bucket. Erasure unlinks immediately with no tombstones, so churn does not degrade performance. Construction, `clear()`, and destruction of non-trivial entries are O(N).
- **Determinism:** LFU ties are broken by recency, so the eviction order is fully determined by the access sequence. Counts saturate at the maximum of `Count`; a saturated entry still refreshes its recency within the top bucket.
- **Ownership:** the cache is non-copyable and non-movable.
- **Thread safety:** none. Guard shared instances with an external lock.
- **Custom policies:** a policy tag must expose `template <size_type N> using state = S;`, where `S` is default-constructible and provides `reset()`, `on_insert(slot)`, `on_access(slot)`, `on_erase(slot)`, and `size_type victim() const`. The contract is documented in [basic_cache.hpp](../../../include/castle/cache/basic_cache.hpp).
