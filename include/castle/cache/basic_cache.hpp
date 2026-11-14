// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file basic_cache.hpp
 * @brief Fixed-capacity key/value cache engine parameterized by an eviction policy.
 *
 * Use this header when a bounded key/value cache is needed without dynamic allocation and the
 * eviction rule should be selectable at compile time. `basic_cache` owns the entry storage and a
 * chained hash index; the `Policy` decides which entry is evicted when a new key arrives while the
 * cache is full. The ready-made policies live in `fifo.hpp`, `lifo.hpp`, `lru.hpp`, `mru.hpp`, and
 * `lfu.hpp`, each of which also provides a short alias such as `castle::cache::lru<Key, T, N>`.
 *
 * Every index structure is an array of compact slot indices embedded in the cache object, so
 * lookups, insertions, erasures, and evictions are O(1) on average and O(N) only when every key
 * collides into the same hash bucket. Erasure unlinks the slot immediately; there are no
 * tombstones, so performance does not degrade under continuous insert/evict churn.
 *
 * @code
 * #include "castle/cache/lru.hpp"
 *
 * castle::cache::lru<uint16_t, int32_t, 4U> cache;
 * cache.put(1U, 10);
 * int32_t* hit = cache.get(1U); // refreshes key 1 for LRU
 * @endcode
 *
 * @note Entries never move while cached; pointers returned by `get()`/`peek()`/`put()` stay valid
 * until that entry is erased, evicted, or the cache is cleared.
 * @warning Capacity is fixed at compile time; `put()` of a new key into a full cache always evicts
 * exactly one entry chosen by the policy.
 */
#ifndef CASTLE_CACHE_BASIC_CACHE_HPP
#define CASTLE_CACHE_BASIC_CACHE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/core/status.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/utility/compare.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/hash.hpp"
#include "castle/utility/move.hpp"

#include <stdint.h>

namespace castle
{
namespace cache
{
namespace detail
{

/**
 * @brief Eviction callback that ignores the evicted entry.
 */
struct ignore_eviction
{
    /**
     * @brief Accepts and discards an evicted entry.
     */
    template <typename Key, typename T>
    void operator()(CASTLE_CONST Key&, T&) CASTLE_CONST CASTLE_NOEXCEPT
    {
    }
};

/**
 * @brief Doubly linked insertion/recency order shared by the FIFO, LIFO, LRU, and MRU policies.
 * @tparam N Cache capacity.
 * @tparam ReorderOnAccess `true` moves an accessed slot to the newest end (LRU/MRU); `false`
 * keeps pure insertion order (FIFO/LIFO).
 * @tparam EvictNewest `true` evicts the newest end (LIFO/MRU); `false` evicts the oldest end
 * (FIFO/LRU).
 * @note The list runs from oldest (`head_`) to newest (`tail_`). Every hook is O(1).
 */
template <size_type N, bool ReorderOnAccess, bool EvictNewest>
class linked_order
{
    using index_type = meta::smallest_uint_t<N>;
    static CASTLE_CONSTEXPR index_type npos = static_cast<index_type>(N);

public:
    /**
     * @brief Constructs an empty order.
     */
    linked_order() CASTLE_NOEXCEPT : prev_{}, next_{}, head_(npos), tail_(npos)
    {
    }

    /**
     * @brief Forgets every tracked slot.
     */
    void reset() CASTLE_NOEXCEPT
    {
        head_ = npos;
        tail_ = npos;
    }

    /**
     * @brief Records a newly inserted slot as the newest entry.
     * @param slot Slot index of the new entry.
     */
    void on_insert(size_type slot) CASTLE_NOEXCEPT
    {
        push_back(static_cast<index_type>(slot));
    }

    /**
     * @brief Records a read or write of an existing slot.
     * @param slot Slot index of the accessed entry.
     * @note Moves the slot to the newest end only when `ReorderOnAccess` is `true`.
     */
    void on_access(CASTLE_UNUSED size_type slot) CASTLE_NOEXCEPT
    {
        if CASTLE_CONSTEXPR (ReorderOnAccess)
        {
            CASTLE_CONST index_type index = static_cast<index_type>(slot);
            if (index != tail_)
            {
                unlink(index);
                push_back(index);
            }
        }
    }

    /**
     * @brief Stops tracking an erased or evicted slot.
     * @param slot Slot index being removed.
     */
    void on_erase(size_type slot) CASTLE_NOEXCEPT
    {
        unlink(static_cast<index_type>(slot));
    }

    /**
     * @brief Returns the slot that should be evicted next.
     * @return Newest slot when `EvictNewest` is `true`; otherwise the oldest slot.
     * @warning Valid only while at least one slot is tracked.
     */
    size_type victim() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if CASTLE_CONSTEXPR (EvictNewest)
        {
            return tail_;
        }
        else
        {
            return head_;
        }
    }

private:
    void push_back(index_type index) CASTLE_NOEXCEPT
    {
        prev_[index] = tail_;
        next_[index] = npos;
        if (tail_ == npos)
        {
            head_ = index;
        }
        else
        {
            next_[tail_] = index;
        }
        tail_ = index;
    }

    void unlink(index_type index) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type before = prev_[index];
        CASTLE_CONST index_type after = next_[index];
        if (before == npos)
        {
            head_ = after;
        }
        else
        {
            next_[before] = after;
        }
        if (after == npos)
        {
            tail_ = before;
        }
        else
        {
            prev_[after] = before;
        }
    }

    index_type prev_[N];
    index_type next_[N];
    index_type head_;
    index_type tail_;
};

} // namespace detail

/**
 * @brief Fixed-capacity key/value cache whose eviction order is chosen by `Policy`.
 * @tparam Key Key type. Must be copy-constructible.
 * @tparam T Cached value type.
 * @tparam N Maximum number of cached entries.
 * @tparam Policy Eviction policy tag (`fifo_policy`, `lifo_policy`, `lru_policy`, `mru_policy`,
 * `lfu_policy<>`, or a user type with the same shape, see the note below).
 * @tparam Hash Hash functor used to select a bucket for a key.
 * @tparam KeyEqual Equality functor used to compare keys.
 * @note A policy tag exposes `template <size_type N> using state = S;` where `S` is
 * default-constructible (empty) and provides `reset()`, `on_insert(slot)`, `on_access(slot)`,
 * `on_erase(slot)`, and `size_type victim() const`. The cache calls `victim()` only while at
 * least one entry exists, and calls `on_insert()` only after a free slot has been claimed.
 * @warning The cache is non-copyable and non-movable because it owns in-place entry storage.
 * It is not thread-safe; guard shared instances with an external lock.
 */
template <typename Key,
          typename T,
          size_type N,
          typename Policy,
          typename Hash = castle::hash<Key>,
          typename KeyEqual = castle::equal_to<Key>>
class basic_cache
{
    static_assert(N > 0U, "basic_cache requires a positive capacity");

    using index_type = meta::smallest_uint_t<N>;
    using policy_state = typename Policy::template state<N>;
    static CASTLE_CONSTEXPR index_type npos = static_cast<index_type>(N);

    struct entry
    {
        template <typename ValueArg>
        entry(CASTLE_CONST Key& entry_key, ValueArg&& entry_value)
            : key(entry_key), value(CASTLE_FORWARD<ValueArg>(entry_value))
        {
        }

        CASTLE_CONST Key key;
        T value;
    };

public:
    using key_type = Key;
    using mapped_type = T;
    using size_type = castle::size_type;
    using policy_type = Policy;
    using hasher = Hash;
    using key_equal = KeyEqual;

    /**
     * @brief Constructs an empty cache with default hash and equality functors.
     * @note Complexity is O(N).
     */
    basic_cache() CASTLE_NOEXCEPT : basic_cache(Hash(), KeyEqual())
    {
    }

    /**
     * @brief Constructs an empty cache with caller-supplied functors.
     * @param hash Hash functor used for bucket selection.
     * @param equal Equality functor used for key comparison.
     * @note Complexity is O(N).
     */
    explicit basic_cache(CASTLE_CONST Hash& hash, CASTLE_CONST KeyEqual& equal = KeyEqual()) CASTLE_NOEXCEPT
        : size_(0U), free_(0U), hash_(hash), equal_(equal), policy_()
    {
        reset_index();
    }

    basic_cache(CASTLE_CONST basic_cache&) CASTLE_DELETE;
    basic_cache& operator=(CASTLE_CONST basic_cache&) CASTLE_DELETE;
    basic_cache(basic_cache&&) CASTLE_DELETE;
    basic_cache& operator=(basic_cache&&) CASTLE_DELETE;

    /**
     * @brief Destroys every cached entry without invoking any eviction callback.
     * @note Complexity is O(N).
     */
    ~basic_cache() CASTLE_NOEXCEPT { destroy_all(); }

    /**
     * @brief Returns the fixed compile-time entry capacity.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the number of cached entries.
     * @return Current entry count.
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Tests whether the cache holds no entries.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Tests whether the next `put()` of a new key will evict an entry.
     * @return `true` when `size() == capacity()`.
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Inserts or updates `key` with a copy of `value`.
     * @param key Key to insert or update.
     * @param value Value to store.
     * @return Reference to the cached value.
     * @note Updating an existing key counts as an access. Inserting into a full cache first
     * evicts the policy's victim. Average complexity is O(1).
     * @warning `value` must not refer to a value stored in this cache, because the referenced
     * entry may be evicted before the copy is made.
     */
    T& put(CASTLE_CONST Key& key, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        detail::ignore_eviction on_evict;
        return put_value(key, value, on_evict);
    }

    /**
     * @brief Inserts or updates `key` by moving `value` into the cache.
     * @param key Key to insert or update.
     * @param value Value to store.
     * @return Reference to the cached value.
     * @note Same semantics and complexity as the copying overload.
     */
    T& put(CASTLE_CONST Key& key, T&& value) CASTLE_NOEXCEPT
    {
        detail::ignore_eviction on_evict;
        return put_value(key, CASTLE_MOVE(value), on_evict);
    }

    /**
     * @brief Inserts or updates `key` with a copy of `value`, reporting any eviction.
     * @tparam OnEvict Callable as `void(const Key&, T&)`.
     * @param key Key to insert or update.
     * @param value Value to store.
     * @param on_evict Called with the victim entry immediately before it is destroyed; it is not
     * called when no eviction is needed.
     * @return Reference to the cached value.
     * @warning `on_evict` must not modify this cache. `value` must not refer to a cached value.
     */
    template <typename OnEvict>
    T& put(CASTLE_CONST Key& key, CASTLE_CONST T& value, OnEvict&& on_evict) CASTLE_NOEXCEPT
    {
        return put_value(key, value, on_evict);
    }

    /**
     * @brief Inserts or updates `key` by moving `value`, reporting any eviction.
     * @tparam OnEvict Callable as `void(const Key&, T&)`.
     * @param key Key to insert or update.
     * @param value Value to store.
     * @param on_evict Called with the victim entry immediately before it is destroyed.
     * @return Reference to the cached value.
     * @warning `on_evict` must not modify this cache.
     */
    template <typename OnEvict>
    T& put(CASTLE_CONST Key& key, T&& value, OnEvict&& on_evict) CASTLE_NOEXCEPT
    {
        return put_value(key, CASTLE_MOVE(value), on_evict);
    }

    /**
     * @brief Looks up `key` and records the access with the policy.
     * @param key Key to search for.
     * @return Pointer to the cached value, or `nullptr` on a miss.
     * @note Average complexity is O(1).
     */
    T* get(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type index = find_index(key);
        if (index == npos)
        {
            return nullptr;
        }
        policy_.on_access(index);
        return &entry_at(index)->value;
    }

    /**
     * @brief Looks up `key` without changing the eviction order.
     * @param key Key to search for.
     * @return Pointer to the cached value, or `nullptr` on a miss.
     */
    T* peek(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type index = find_index(key);
        return index == npos
               ? nullptr
               : &entry_at(index)->value;
    }

    /**
     * @brief Looks up `key` without changing the eviction order.
     * @param key Key to search for.
     * @return Const pointer to the cached value, or `nullptr` on a miss.
     */
    CASTLE_CONST T* peek(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type index = find_index(key);
        return index == npos
               ? nullptr
               : &entry_at(index)->value;
    }

    /**
     * @brief Tests whether `key` is cached without changing the eviction order.
     * @param key Key to search for.
     * @return `true` when the key is cached.
     */
    bool contains(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_index(key) != npos;
    }

    /**
     * @brief Removes `key` from the cache without invoking any eviction callback.
     * @param key Key to remove.
     * @return `status::ok` on success or `status::not_found` when the key is not cached.
     */
    status erase(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type index = find_index(key);
        if (index == npos)
        {
            return status::not_found;
        }
        remove_at(index);
        return status::ok;
    }

    /**
     * @brief Removes the entry the policy would evict next.
     * @return `status::ok` on success or `status::empty` when the cache holds no entries.
     */
    status evict() CASTLE_NOEXCEPT
    {
        detail::ignore_eviction on_evict;
        return evict(on_evict);
    }

    /**
     * @brief Removes the entry the policy would evict next, reporting it first.
     * @tparam OnEvict Callable as `void(const Key&, T&)`.
     * @param on_evict Called with the victim entry immediately before it is destroyed.
     * @return `status::ok` on success or `status::empty` when the cache holds no entries.
     * @note Repeated calls drain the cache in eviction order, which is useful for write-back
     * flushing.
     * @warning `on_evict` must not modify this cache.
     */
    template <typename OnEvict>
    status evict(OnEvict&& on_evict) CASTLE_NOEXCEPT
    {
        if (size_ == 0U)
        {
            return status::empty;
        }
        evict_victim(on_evict);
        return status::ok;
    }

    /**
     * @brief Destroys every entry without invoking any eviction callback.
     * @note Complexity is O(N).
     */
    void clear() CASTLE_NOEXCEPT
    {
        destroy_all();
        reset_index();
        policy_.reset();
    }

private:
    template <typename ValueArg, typename OnEvict>
    T& put_value(CASTLE_CONST Key& key, ValueArg&& value, OnEvict& on_evict) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type found = find_index(key);
        if (found != npos)
        {
            entry* existing = entry_at(found);
            existing->value = CASTLE_FORWARD<ValueArg>(value);
            policy_.on_access(found);
            return existing->value;
        }
        if (size_ == N)
        {
            evict_victim(on_evict);
        }

        CASTLE_CONST index_type index = free_;
        free_ = chain_[index];
        entry* created = memory::construct_at<entry>(slots_.address(index), key, CASTLE_FORWARD<ValueArg>(value));
        index_type& head = buckets_[bucket(key)];
        chain_[index] = head;
        head = index;
        ++size_;
        policy_.on_insert(index);
        return created->value;
    }

    template <typename OnEvict>
    void evict_victim(OnEvict& on_evict) CASTLE_NOEXCEPT
    {
        CASTLE_CONST index_type index = static_cast<index_type>(policy_.victim());
        entry* victim = entry_at(index);
        on_evict(victim->key, victim->value);
        remove_at(index);
    }

    void remove_at(index_type index) CASTLE_NOEXCEPT
    {
        entry* removed = entry_at(index);
        index_type* link = &buckets_[bucket(removed->key)];
        while (*link != index)
        {
            link = &chain_[*link];
        }
        *link = chain_[index];

        policy_.on_erase(index);
        memory::destroy_at(removed);
        chain_[index] = free_;
        free_ = index;
        --size_;
    }

    index_type find_index(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        index_type index = buckets_[bucket(key)];
        while ((index != npos) && !equal_(entry_at(index)->key, key))
        {
            index = chain_[index];
        }
        return index;
    }

    size_type bucket(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash_(key) % N;
    }

    void reset_index() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            buckets_[i] = npos;
            chain_[i] = static_cast<index_type>(i + 1U); // Free list 0 -> 1 -> ... -> N-1 -> npos.
        }
        free_ = 0U;
        size_ = 0U;
    }

    void destroy_all() CASTLE_NOEXCEPT
    {
        if CASTLE_CONSTEXPR (!meta::is_trivially_destructible<entry>::value)
        {
            for (size_type i = 0U; i < N; ++i)
            {
                index_type index = buckets_[i];
                while (index != npos)
                {
                    CASTLE_CONST index_type next = chain_[index];
                    memory::destroy_at(entry_at(index));
                    index = next;
                }
            }
        }
    }

    entry* entry_at(index_type index) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<entry>(slots_.address(index));
    }

    CASTLE_CONST entry* entry_at(index_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return memory::object_from_address<entry>(slots_.address(index));
    }

    index_type size_;
    index_type free_;
    Hash hash_;
    KeyEqual equal_;
    policy_state policy_;
    index_type buckets_[N];
    index_type chain_[N]; // Bucket-chain link for live slots; free-list link for free slots.
    memory::static_storage<entry, N> slots_;
};

} // namespace cache
} // namespace castle

#endif // CASTLE_CACHE_BASIC_CACHE_HPP
