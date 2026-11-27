// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file top_k.hpp
 * @brief Fixed-size top-k summary (Space-Saving): the most frequent keys of a stream in `K` slots.
 *
 * Use `top_k<Key, K>` when a stream holds far more distinct keys than RAM allows to count, but
 * only the heaviest ones matter: top talkers on a CAN bus or network port, the most frequent
 * DTCs or error codes, hot flash pages, the busiest interrupt sources, or the most requested
 * resources. The summary tracks at most `K` keys with a count and an error each.
 *
 * Space-Saving (Metwally, Agrawal, El Abbadi) always admits a new key: when every slot is used,
 * the key with the smallest count is replaced and the newcomer inherits that count as its error.
 * Let `N = total()` and `m = min_count()`. For every tracked entry and every key `x` with true
 * count `f(x)`:
 * - `entry.count - entry.error <= f(x) <= entry.count` (deterministic, no probability);
 * - an untracked key has `f(x) <= m`, and `m <= N / K`;
 * - so every key with `f(x) > N / K` is tracked.
 *
 * Storage is a min-heap on the count, so eviction is O(log K). Key lookup is a linear scan over
 * the `K` slots: no hash and no extra index, which is the fastest choice for the small `K` of
 * embedded systems.
 *
 * @code
 * using talkers_type = castle::dsa::top_k<uint32_t, 8U>; // the 8 busiest CAN ids
 * talkers_type talkers;
 * talkers.add(0x18DAF110U);
 * talkers_type::entry_type best[3];
 * castle::size_type n = talkers.top(castle::dsa::array_view<talkers_type::entry_type>(best, 3U)); // highest first
 * @endcode
 */
#ifndef CASTLE_DSA_TOP_K_HPP
#define CASTLE_DSA_TOP_K_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle/dsa/array.hpp"
#include "castle/dsa/array_view.hpp"
#include "castle/math/saturating.hpp"
#include "castle/utility/compare.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Fixed-size Space-Saving summary of the `K` most frequent keys.
 * @tparam Key Key type. Must be default-constructible and copy-assignable.
 * @tparam K Number of tracked keys. Must be positive.
 * @tparam Counter Unsigned integral counter type. Counts saturate at its maximum.
 * @tparam KeyEqual Equality functor for keys. Its `operator()` must be `const`.
 * @note Storage is one `dsa::array<entry_type, K>`, one size, one `uint64_t` stream total, and one
 * `KeyEqual` object. The summary is copyable.
 * @note `add()`, `estimate()`, `find()`, and `contains()` are O(K) (linear lookup) plus O(log K)
 * for the heap update. `top()` is O(K log K). `halve()` is O(K).
 * @warning A tracked key is a copy of the caller's key. View keys such as `string_view` must
 * outlive the summary.
 */
template <typename Key,
          size_type K,
          typename Counter = uint32_t,
          typename KeyEqual = castle::equal_to<Key>>
class top_k
{
    static_assert(K > 0U, "dsa::top_k requires a positive capacity");
    static_assert(K <= static_cast<castle::size_type>(-1) / 2U, "dsa::top_k capacity is too large");
    static_assert(meta::is_integral<Counter>::value && meta::is_unsigned<Counter>::value &&
                      !meta::is_same<Counter, bool>::value,
                  "dsa::top_k requires an unsigned integral counter type");

public:
    using key_type = Key;
    using counter_type = Counter;
    using key_equal = KeyEqual;
    using size_type = castle::size_type;

    /**
     * @brief One tracked key with its estimated count.
     * @note `count - error <= true count <= count`.
     */
    struct entry_type
    {
        key_type key;       /**< Tracked key. */
        counter_type count; /**< Upper bound of the true count. */
        counter_type error; /**< Count inherited from the evicted key: the maximum overestimate. */

        /** @brief Returns the guaranteed lower bound `count - error` of the true count. */
        CASTLE_NODISCARD counter_type guaranteed() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return static_cast<counter_type>(count - error);
        }
    };

    using const_iterator = CASTLE_CONST entry_type*;

    /** @brief Constructs an empty summary with a default-constructed `KeyEqual`. */
    top_k() CASTLE_NOEXCEPT
        : entries_(), size_(0U), total_(0U), equal_()
    {
    }

    /**
     * @brief Constructs an empty summary with a caller-supplied key equality functor.
     * @param equal Equality functor copied into the summary.
     */
    explicit top_k(CASTLE_CONST KeyEqual& equal) CASTLE_NOEXCEPT
        : entries_(), size_(0U), total_(0U), equal_(equal)
    {
    }

    /** @brief Returns the number of key slots, `K`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return K;
    }

    /** @brief Returns the number of tracked keys, `0 .. K`. */
    CASTLE_NODISCARD size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /** @brief Reports whether no key is tracked. */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

    /** @brief Reports whether every slot is used, so the next new key evicts the minimum. */
    CASTLE_NODISCARD bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == K;
    }

    /** @brief Returns the sum `N` of every added count, saturated at `UINT64_MAX`. */
    CASTLE_NODISCARD uint64_t total() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return total_;
    }

    /**
     * @brief Returns the upper bound of the true count of every untracked key.
     * @return The smallest tracked count when full; zero otherwise, because no key was evicted.
     * @note Never exceeds `total() / K` while no count saturates. Complexity: O(1).
     */
    CASTLE_NODISCARD counter_type min_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return full() ? entries_[0U].count : counter_type{0};
    }

    /**
     * @brief Adds `count` occurrences of a key.
     * @param key Key to count.
     * @param count Number of occurrences. Zero changes nothing and evicts nothing.
     * @return `estimate(key)` after the call. For `count > 0` that is the tracked count of `key`.
     * @note A new key takes a free slot, or replaces the entry with the smallest count and
     * inherits that count as its error. Counts saturate. Complexity: O(K).
     */
    counter_type add(CASTLE_CONST key_type& key, counter_type count = 1U) CASTLE_NOEXCEPT
    {
        if (count == 0U)
        {
            return estimate(key);
        }
        total_ = math::saturating_add<uint64_t>(total_, static_cast<uint64_t>(count));

        CASTLE_CONST size_type index = index_of(key);
        if (index != npos)
        {
            entry_type& tracked = entries_[index];
            tracked.count = math::saturating_add<counter_type>(tracked.count, count);
            CASTLE_CONST counter_type result = tracked.count;
            restore_heap(index);
            return result;
        }

        if (!full())
        {
            entries_[size_] = entry_type{key, count, counter_type{0}};
            ++size_;
            if (full())
            {
                make_heap();
            }
            return count;
        }

        // Space-Saving eviction: the newcomer replaces the minimum and inherits its count as error.
        entry_type& minimum = entries_[0U];
        CASTLE_CONST counter_type inherited = minimum.count;
        minimum.key = key;
        minimum.error = inherited;
        minimum.count = math::saturating_add<counter_type>(inherited, count);
        CASTLE_CONST counter_type result = minimum.count;
        restore_heap(0U);
        return result;
    }

    /**
     * @brief Returns an upper bound of the true count of a key.
     * @param key Key to look up.
     * @return The tracked count, or `min_count()` when `key` is not tracked.
     * @note Complexity: O(K).
     */
    CASTLE_NODISCARD counter_type estimate(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = index_of(key);
        return (index != npos) ? entries_[index].count : min_count();
    }

    /**
     * @brief Returns the entry of a tracked key.
     * @param key Key to look up.
     * @return Pointer to the entry, or `nullptr` when `key` is not tracked. The pointer stays valid
     * until the next `add()`, `halve()`, `top()`, or `clear()`.
     * @note Complexity: O(K).
     */
    CASTLE_NODISCARD CASTLE_CONST entry_type* find(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = index_of(key);
        return (index != npos) ? &entries_[index] : nullptr;
    }

    /** @brief Reports whether a key is tracked. Complexity: O(K). */
    CASTLE_NODISCARD bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_of(key) != npos;
    }

    /**
     * @brief Copies the heaviest entries, highest count first.
     * @param out Destination; receives `min(out.size(), size())` entries.
     * @return Number of entries written.
     * @note Reorders the internal storage (tracked counts are unchanged), so pointers from
     * `find()` and iterators become invalid. Equal counts come out in an unspecified but
     * deterministic order. Complexity: O(K log K).
     */
    size_type top(array_view<entry_type> out) CASTLE_NOEXCEPT
    {
        // An ascending order is also a valid min-heap, so sorting keeps the summary consistent.
        castle::sort(entries_.data(), entries_.data() + size_, count_less());
        CASTLE_CONST size_type written = (out.size() < size_) ? out.size() : size_;
        for (size_type i = 0U; i < written; ++i)
        {
            out[i] = entries_[size_ - 1U - i];
        }
        return written;
    }

    /**
     * @brief Halves every count, rounding down, to age old traffic.
     * @note Call once per time window so recent keys dominate. Each entry keeps
     * `count - error <= floor(f / 2) <= count` for the true count `f` seen before the call.
     * Complexity: O(K).
     */
    void halve() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < size_; ++i)
        {
            entry_type& tracked = entries_[i];
            CASTLE_CONST counter_type lower = static_cast<counter_type>(tracked.guaranteed() >> 1U);
            tracked.count = static_cast<counter_type>(tracked.count >> 1U);
            tracked.error = static_cast<counter_type>(tracked.count - lower);
        }
        // Halving is monotonic, so the heap order survives.
        total_ >>= 1U;
    }

    /** @brief Forgets every key and the total. Complexity: O(1). */
    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
        total_ = 0U;
    }

    /** @brief Returns the first tracked entry. Entries are in internal (heap) order. */
    CASTLE_NODISCARD const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return entries_.data();
    }

    /** @brief Returns one past the last tracked entry. */
    CASTLE_NODISCARD const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return entries_.data() + size_;
    }

    /** @brief Returns the stored key equality functor. */
    CASTLE_NODISCARD CASTLE_CONST key_equal& key_eq() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return equal_;
    }

private:
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    /** @brief Orders entries by ascending count, for `castle::sort`. */
    struct count_less
    {
        bool operator()(CASTLE_CONST entry_type& lhs, CASTLE_CONST entry_type& rhs) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return lhs.count < rhs.count;
        }
    };

    /** @brief Sifts larger counts down, for `castle::detail::heap_sift_down`: a min-heap on the count. */
    struct count_greater
    {
        bool operator()(CASTLE_CONST entry_type& lhs, CASTLE_CONST entry_type& rhs) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return lhs.count > rhs.count;
        }
    };

    size_type index_of(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < size_; ++i)
        {
            if (equal_(entries_[i].key, key))
            {
                return i;
            }
        }
        return npos;
    }

    /** @brief Builds the min-heap once all `K` slots are used. Before that, no key is evicted. */
    void make_heap() CASTLE_NOEXCEPT
    {
        for (size_type start = K / 2U; start > 0U; --start)
        {
            sift_down(start - 1U);
        }
    }

    /** @brief Restores the heap after the count at `index` grew. */
    void restore_heap(size_type index) CASTLE_NOEXCEPT
    {
        if (full())
        {
            sift_down(index);
        }
    }

    void sift_down(size_type index) CASTLE_NOEXCEPT
    {
        castle::detail::heap_sift_down(entries_.data(),
                                       static_cast<difference_type>(index),
                                       static_cast<difference_type>(K),
                                       count_greater());
    }

    array<entry_type, K> entries_;
    size_type size_;
    uint64_t total_;
    KeyEqual equal_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_TOP_K_HPP
