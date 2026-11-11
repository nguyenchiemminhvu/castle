// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mru.hpp
 * @brief Fixed-capacity MRU (most recently used) cache.
 *
 * Use this header for cyclic or sequential scan workloads that are larger than the cache, where
 * the entry just used is the one least likely to be needed again soon. Every `get()` hit and every
 * `put()` (insert or update) marks the entry as most recently used, and that entry is evicted
 * first, so older entries are preserved across repeated scans.
 *
 * @code
 * #include "castle/cache/mru.hpp"
 *
 * castle::cache::mru<uint8_t, int32_t, 2U> cache;
 * cache.put(1U, 10);
 * cache.put(2U, 20);
 * cache.get(1U);     // key 1 becomes most recently used
 * cache.put(3U, 30); // evicts key 1
 * @endcode
 *
 * @note All operations are O(1) on average; see `basic_cache.hpp` for the full API.
 */
#ifndef CASTLE_CACHE_MRU_HPP
#define CASTLE_CACHE_MRU_HPP

#include "castle/cache/basic_cache.hpp"

namespace castle
{
namespace cache
{

/**
 * @brief Eviction policy tag that evicts the most recently used entry.
 * @note `get()` hits and every `put()` refresh an entry; `peek()` and `contains()` do not.
 */
struct mru_policy
{
    /**
     * @brief Per-cache policy state for capacity `N`.
     * @tparam N Cache capacity.
     */
    template <size_type N>
    using state = detail::linked_order<N, true, true>;
};

/**
 * @brief Most Recently Used cache that evicts the most recently used entry when full.
 * @tparam Key Key type.
 * @tparam T Cached value type.
 * @tparam N Maximum number of cached entries.
 * @tparam Hash Hash functor used to select a bucket for a key.
 * @tparam KeyEqual Equality functor used to compare keys.
 */
template <typename Key,
          typename T,
          size_type N,
          typename Hash = castle::hash<Key>,
          typename KeyEqual = castle::equal_to<Key>>
using mru = basic_cache<Key, T, N, mru_policy, Hash, KeyEqual>;

} // namespace cache
} // namespace castle

#endif // CASTLE_CACHE_MRU_HPP
