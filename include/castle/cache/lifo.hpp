// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file lifo.hpp
 * @brief Fixed-capacity LIFO (last-in, first-out) cache.
 *
 * Use this header when the cache should behave like a bounded stack: the most recently inserted
 * entry is evicted first, so the earliest entries stay resident. Reading or updating an entry
 * never changes its position. This keeps a stable "base set" of early entries, for example
 * boot-time configuration that must survive while transient entries rotate above it.
 *
 * @code
 * #include "castle/cache/lifo.hpp"
 *
 * castle::cache::lifo<uint8_t, int32_t, 2U> cache;
 * cache.put(1U, 10);
 * cache.put(2U, 20);
 * cache.put(3U, 30); // evicts key 2, the most recently inserted
 * @endcode
 *
 * @note All operations are O(1) on average; see `basic_cache.hpp` for the full API.
 */
#ifndef CASTLE_CACHE_LIFO_HPP
#define CASTLE_CACHE_LIFO_HPP

#include "castle/cache/basic_cache.hpp"

namespace castle
{
namespace cache
{

/**
 * @brief Eviction policy tag that evicts the most recently inserted entry.
 * @note `get()` and updating `put()` do not change an entry's position.
 */
struct lifo_policy
{
    /**
     * @brief Per-cache policy state for capacity `N`.
     * @tparam N Cache capacity.
     */
    template <size_type N>
    using state = detail::linked_order<N, false, true>;
};

/**
 * @brief Last-In, First-Out cache that evicts the most recently inserted entry when full.
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
using lifo = basic_cache<Key, T, N, lifo_policy, Hash, KeyEqual>;

} // namespace cache
} // namespace castle

#endif // CASTLE_CACHE_LIFO_HPP
