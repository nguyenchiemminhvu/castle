# Cache

## Overview
Umbrella header for the Castle cache module. It includes the [`basic_cache`](basic_cache.md) engine and all five eviction policies, so one include gives access to every cache type. Use it when you want to compare policies or need several of them together.

## Header
`#include "castle/cache/cache.hpp"`

## Dependencies
- [`basic_cache.md`](basic_cache.md)
- [`fifo.md`](fifo.md)
- [`lifo.md`](lifo.md)
- [`lru.md`](lru.md)
- [`mru.md`](mru.md)
- [`lfu.md`](lfu.md)

## Public API
This header adds no new declarations. It re-exports:

| Alias | Evicts when full | Access (`get()` hit / updating `put()`) |
|---|---|---|
| `castle::cache::fifo<Key, T, N>` | Earliest inserted | Ignored |
| `castle::cache::lifo<Key, T, N>` | Most recently inserted | Ignored |
| `castle::cache::lru<Key, T, N>` | Least recently used | Refreshes recency |
| `castle::cache::mru<Key, T, N>` | Most recently used | Refreshes recency |
| `castle::cache::lfu<Key, T, N>` | Lowest access count, LRU among ties | Increments count |

All aliases share the `basic_cache` API: `put`, `get`, `peek`, `contains`, `erase`, `evict`, `clear`, `size`, `capacity`, `empty`, `full`.

## Usage Example
See [`samples/cache/cache.cpp`](../../samples/cache/cache.cpp).

```cpp
#include "castle/cache/cache.hpp"

castle::cache::lru<uint8_t, uint8_t, 3U> recent;
castle::cache::lfu<uint8_t, uint8_t, 3U> popular;
recent.put(1U, 10U);
popular.put(1U, 10U);
```

## Constraints & Notes
- Convenience include only. Include the single policy header you use when compile-time dependency size matters.
- Allocation, complexity, and thread-safety rules are those of [`basic_cache`](basic_cache.md).
