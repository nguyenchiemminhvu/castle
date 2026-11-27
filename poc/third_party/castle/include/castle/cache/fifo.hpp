// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fifo.hpp
 * @brief Fixed-capacity FIFO (first-in, first-out) cache.
 *
 * Use this header when entries should leave the cache in the order they arrived, regardless of how
 * often or how recently they were read or written. The oldest inserted entry is evicted first.
 * Reading or updating an entry never changes its position, so FIFO is the cheapest policy and the
 * easiest to reason about for streaming or time-ordered data.
 *
 * @code
 * #include "castle/cache/fifo.hpp"
 *
 * castle::cache::fifo<uint8_t, int32_t, 2U> cache;
 * cache.put(1U, 10);
 * cache.put(2U, 20);
 * cache.get(1U);     // FIFO ignores accesses
 * cache.put(3U, 30); // evicts key 1, the earliest inserted
 * @endcode
 *
 * @note All operations are O(1) on average; see `basic_cache.hpp` for the full API.
 */
#ifndef CASTLE_CACHE_FIFO_HPP
#define CASTLE_CACHE_FIFO_HPP

#include "castle/cache/basic_cache.hpp"

namespace castle
{
namespace cache
{

/**
 * @brief Eviction policy tag that evicts the earliest inserted entry.
 * @note `get()` and updating `put()` do not change an entry's position.
 */
struct fifo_policy
{
    /**
     * @brief Per-cache policy state for capacity `N`.
     * @tparam N Cache capacity.
     */
    template <size_type N>
    using state = detail::linked_order<N, false, false>;
};

/**
 * @brief First-In, First-Out cache that evicts the earliest inserted entry when full.
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
using fifo = basic_cache<Key, T, N, fifo_policy, Hash, KeyEqual>;

} // namespace cache
} // namespace castle

#endif // CASTLE_CACHE_FIFO_HPP
