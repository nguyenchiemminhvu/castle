// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file lfu.hpp
 * @brief Fixed-capacity LFU (least frequently used) cache with LRU tie-breaking.
 *
 * Use this header when long-term popularity predicts future use better than recency. Every entry
 * carries an access count: insertion starts it at 1, and every `get()` hit or updating `put()`
 * adds 1. When the cache is full, the entry with the lowest count is evicted; among entries that
 * share the lowest count, the least recently used one is evicted, so the order is deterministic.
 *
 * Entries are grouped into frequency buckets kept in ascending count order, and each bucket keeps
 * its entries in recency order. Promoting an entry only ever moves it to the adjacent bucket, so
 * every policy hook is O(1) (the classic constant-time LFU scheme), with all bookkeeping stored in
 * fixed arrays inside the cache object.
 *
 * @code
 * #include "castle/cache/lfu.hpp"
 *
 * castle::cache::lfu<uint8_t, int32_t, 2U> cache;
 * cache.put(1U, 10);
 * cache.put(2U, 20);
 * cache.get(1U);     // key 1 count = 2, key 2 count = 1
 * cache.put(3U, 30); // evicts key 2, the least frequently used
 * @endcode
 *
 * @note Counts saturate at the maximum of the counter type; a saturated entry still refreshes its
 * recency within its bucket. Use `lfu_policy<uint8_t>` or `lfu_policy<uint16_t>` to shrink the
 * per-entry counter when saturation is acceptable.
 */
#ifndef CASTLE_CACHE_LFU_HPP
#define CASTLE_CACHE_LFU_HPP

#include "castle/cache/basic_cache.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"

#include <stdint.h>

namespace castle
{
namespace cache
{
namespace detail
{

/**
 * @brief Constant-time LFU order: ascending frequency buckets, each in LRU order.
 * @tparam N Cache capacity.
 * @tparam Count Unsigned access-counter type.
 * @note A bucket node exists only while it holds at least one entry, so at most `N` bucket
 * nodes are needed. A new bucket is allocated only when the cache holds fewer than `N` entries
 * (insertion) or when the source bucket keeps another entry (promotion), so the pool never runs
 * dry.
 */
template <size_type N, typename Count>
class frequency_order
{
    static_assert(meta::is_integral<Count>::value && meta::is_unsigned<Count>::value,
                  "lfu_policy requires an unsigned integral counter type");

    using index_type = meta::smallest_uint_t<N>;
    static CASTLE_CONSTEXPR index_type npos = static_cast<index_type>(N);
    static CASTLE_CONSTEXPR Count max_count = numeric_limits<Count>::max();

public:
    /**
     * @brief Constructs an empty order.
     */
    frequency_order() CASTLE_NOEXCEPT
        : prev_{}, next_{}, bucket_of_{}, count_{}, head_{}, tail_{}, bucket_prev_{}, bucket_next_{},
          lowest_(npos), free_(0U)
    {
        reset();
    }

    /**
     * @brief Forgets every tracked slot and returns all bucket nodes to the pool.
     */
    void reset() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            bucket_next_[i] = static_cast<index_type>(i + 1U);
        }
        free_ = 0U;
        lowest_ = npos;
    }

    /**
     * @brief Records a newly inserted slot with an access count of 1.
     * @param slot Slot index of the new entry.
     */
    void on_insert(size_type slot) CASTLE_NOEXCEPT
    {
        if ((lowest_ == npos) || (count_[lowest_] != 1U))
        {
            CASTLE_CONST index_type created = acquire_bucket(1U);
            bucket_prev_[created] = npos;
            bucket_next_[created] = lowest_;
            if (lowest_ != npos)
            {
                bucket_prev_[lowest_] = created;
            }
            lowest_ = created;
        }
        append(lowest_, static_cast<index_type>(slot));
    }

    /**
     * @brief Adds 1 to the slot's access count and marks it most recent within its new count.
     * @param slot Slot index of the accessed entry.
     */
    void on_access(size_type slot) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type index = static_cast<index_type>(slot);
        CASTLE_CONST index_type source = bucket_of_[index];
        if (count_[source] == max_count)
        {
            // Saturated: keep the count, refresh recency inside the top bucket.
            detach(source, index);
            append(source, index);
            return;
        }

        CASTLE_CONST Count promoted = static_cast<Count>(count_[source] + 1U);
        CASTLE_CONST index_type following = bucket_next_[source];
        if ((following != npos) && (count_[following] == promoted))
        {
            detach(source, index);
            append(following, index);
            release_if_empty(source);
            return;
        }
        if (head_[source] == tail_[source])
        {
            // Sole member: the next bucket's count is larger, so bump the bucket in place.
            count_[source] = promoted;
            return;
        }

        CASTLE_CONST index_type created = acquire_bucket(promoted);
        bucket_prev_[created] = source;
        bucket_next_[created] = following;
        bucket_next_[source] = created;
        if (following != npos)
        {
            bucket_prev_[following] = created;
        }
        detach(source, index);
        append(created, index);
    }

    /**
     * @brief Stops tracking an erased or evicted slot.
     * @param slot Slot index being removed.
     */
    void on_erase(size_type slot) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type index = static_cast<index_type>(slot);
        CASTLE_CONST index_type source = bucket_of_[index];
        detach(source, index);
        release_if_empty(source);
    }

    /**
     * @brief Returns the least recently used slot among those with the lowest count.
     * @return Slot index to evict next.
     * @warning Valid only while at least one slot is tracked.
     */
    size_type victim() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return head_[lowest_];
    }

private:
    index_type acquire_bucket(Count count) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type created = free_;
        free_ = bucket_next_[created];
        count_[created] = count;
        head_[created] = npos;
        tail_[created] = npos;
        return created;
    }

    void release_if_empty(index_type bucket) CASTLE_NOEXCEPT
    {
        if (head_[bucket] != npos)
        {
            return;
        }
        CASTLE_CONST index_type before = bucket_prev_[bucket];
        CASTLE_CONST index_type after = bucket_next_[bucket];
        if (before == npos)
        {
            lowest_ = after;
        }
        else
        {
            bucket_next_[before] = after;
        }
        if (after != npos)
        {
            bucket_prev_[after] = before;
        }
        bucket_next_[bucket] = free_;
        free_ = bucket;
    }

    void append(index_type bucket, index_type index) CASTLE_NOEXCEPT
    {
        bucket_of_[index] = bucket;
        prev_[index] = tail_[bucket];
        next_[index] = npos;
        if (tail_[bucket] == npos)
        {
            head_[bucket] = index;
        }
        else
        {
            next_[tail_[bucket]] = index;
        }
        tail_[bucket] = index;
    }

    void detach(index_type bucket, index_type index) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type before = prev_[index];
        CASTLE_CONST index_type after = next_[index];
        if (before == npos)
        {
            head_[bucket] = after;
        }
        else
        {
            next_[before] = after;
        }
        if (after == npos)
        {
            tail_[bucket] = before;
        }
        else
        {
            prev_[after] = before;
        }
    }

    // Per-slot links inside the owning frequency bucket.
    index_type prev_[N];
    index_type next_[N];
    index_type bucket_of_[N];
    // Frequency bucket node pool.
    Count count_[N];
    index_type head_[N];
    index_type tail_[N];
    index_type bucket_prev_[N];
    index_type bucket_next_[N]; // Ascending-count link for live buckets; free-list link otherwise.
    index_type lowest_;
    index_type free_;
};

} // namespace detail

/**
 * @brief Eviction policy tag that evicts the least frequently used entry, LRU among ties.
 * @tparam Count Unsigned access-counter type; counts saturate at its maximum value.
 * @note `get()` hits and updating `put()` calls increment the count; `peek()` and `contains()`
 * do not.
 */
template <typename Count = uint32_t>
struct lfu_policy
{
    /**
     * @brief Per-cache policy state for capacity `N`.
     * @tparam N Cache capacity.
     */
    template <size_type N>
    using state = detail::frequency_order<N, Count>;
};

/**
 * @brief Least Frequently Used cache that evicts the lowest-count entry (LRU among ties) when full.
 * @tparam Key Key type.
 * @tparam T Cached value type.
 * @tparam N Maximum number of cached entries.
 * @tparam Hash Hash functor used to select a bucket for a key.
 * @tparam KeyEqual Equality functor used to compare keys.
 * @note Uses a `uint32_t` counter. Spell `basic_cache<Key, T, N, lfu_policy<Count>>` to choose
 * another counter width.
 */
template <typename Key,
          typename T,
          size_type N,
          typename Hash = castle::hash<Key>,
          typename KeyEqual = castle::equal_to<Key>>
using lfu = basic_cache<Key, T, N, lfu_policy<>, Hash, KeyEqual>;

} // namespace cache
} // namespace castle

#endif // CASTLE_CACHE_LFU_HPP
