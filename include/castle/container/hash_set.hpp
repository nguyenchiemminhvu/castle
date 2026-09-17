// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file hash_set.hpp
 * @brief Fixed-capacity hash set backed by Castle's deterministic hash table.
 *
 * Use `hash_set<Key, N>` when a set of unique keys should live entirely inside
 * the container object and hash-based lookup is preferred over ordered
 * traversal. Capacity is fixed by `N`, no dynamic allocation occurs, and the
 * underlying open-addressed table performs linear probing with O(N) worst-case
 * insertion, lookup, and erasure.
 *
 * @code
 * castle::container::hash_set<int, 4U> ids;
 * ids.insert(7);
 * bool present = ids.contains(7);
 * @endcode
 */
#ifndef CASTLE_CONTAINER_HASH_SET_HPP
#define CASTLE_CONTAINER_HASH_SET_HPP

#include "castle/container/hash_table.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/core/error_handler.hpp"

namespace castle
{
namespace container
{
namespace detail
{
struct hash_set_value
{
};
}

/**
 * @brief Fixed-capacity hash set of unique keys.
 * @tparam Key Key type stored in the set.
 * @tparam N Maximum number of live keys stored without dynamic allocation.
 * @tparam Hash Hash functor used to map keys to buckets.
 * @tparam KeyEqual Equality functor used to compare keys during probing.
 * @note Storage is embedded. Capacity/state queries are O(1).
 * @note `insert()`, `contains()`, `find()`, and `erase()` are O(N) worst-case.
 * @warning Iterators and references to erased elements are invalidated by
 * `erase()`. Other iterators remain valid because entries are never relocated.
 * `clear()` invalidates all iterators.
 */
template <typename Key,
          size_type N,
          typename Hash = castle::hash<Key>,
          typename KeyEqual = castle::equal_to<Key>>
class hash_set
{
    using table_type = hash_table<Key, detail::hash_set_value, N, Hash, KeyEqual>;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;

    /**
     * @brief Forward iterator over set keys.
     * @note Dereferencing yields a const key reference because keys are immutable.
     * @warning Dereferencing or incrementing `end()` is undefined.
     */
    class const_iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = Key;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST Key*;
        using reference = CASTLE_CONST Key&;

        /**
         * @brief Constructs a default iterator.
         * @note Complexity: O(1).
         */
        const_iterator() CASTLE_NOEXCEPT : it_() {}

        /**
         * @brief Wraps the underlying hash-table iterator.
         * @param it Underlying iterator to wrap.
         * @note Complexity: O(1).
         */
        explicit const_iterator(typename table_type::const_iterator it) CASTLE_NOEXCEPT : it_(it) {}

        /**
         * @brief Returns the current key.
         * @return Reference to the current key.
         * @note Complexity: O(1).
         * @warning The iterator must not equal `end()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return it_->first; }

        /**
         * @brief Returns a pointer to the current key.
         * @return Pointer to the current key.
         * @note Complexity: O(1).
         * @warning The iterator must not equal `end()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return &it_->first; }

        /**
         * @brief Advances to the next occupied slot.
         * @return Reference to `*this`.
         * @note Complexity: O(N) worst-case.
         */
        const_iterator& operator++() CASTLE_NOEXCEPT { ++it_; return *this; }

        /**
         * @brief Advances to the next occupied slot and returns the previous iterator.
         * @return Iterator state before incrementing.
         * @note Complexity: O(N) worst-case.
         */
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }

        /**
         * @brief Compares two iterators for equality.
         * @param other Iterator to compare against.
         * @return `true` when both iterators refer to the same position.
         * @note Complexity: O(1).
         */
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return it_ == other.it_; }

        /**
         * @brief Compares two iterators for inequality.
         * @param other Iterator to compare against.
         * @return `true` when the iterators refer to different positions.
         * @note Complexity: O(1).
         */
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }
    private:
        typename table_type::const_iterator it_;
        friend class hash_set;
    };

    using iterator = const_iterator;

    /**
     * @brief Constructs an empty hash set using the default hash functor.
     * @note Complexity: O(1).
     */
    hash_set() CASTLE_NOEXCEPT : table_() {}

    /**
     * @brief Constructs an empty hash set using a caller-supplied hash functor.
     * @param hash Hash functor copied into the underlying table.
     * @note Complexity: O(1).
     */
    explicit hash_set(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT : table_(hash) {}

    /**
     * @brief Constructs a set from an initializer list.
     * @param list Keys inserted in list order.
     * @note Complexity: O(list.size() * N) worst-case.
     * @warning `list.size()` must not exceed `N`; the constructor reports a
     * violated precondition with `CASTLE_ASSERT`. Duplicate keys are ignored.
     */
    hash_set(initializer_list<key_type> list) CASTLE_NOEXCEPT : table_()
    {
        for (CASTLE_CONST key_type& key : list)
        {
            if (full())
            {
                break;
            }
            insert(key);
        }
    }

    /**
     * @brief Returns the compile-time capacity.
     * @return Maximum number of keys storable in the set.
     * @note Complexity: O(1).
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the number of stored keys.
     * @return Current set size.
     * @note Complexity: O(1).
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return table_.size(); }

    /**
     * @brief Returns the remaining insertion capacity.
     * @return `capacity() - size()`.
     * @note Complexity: O(1).
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return table_.available(); }

    /**
     * @brief Reports whether the set is empty.
     * @return `true` when no keys are stored.
     * @note Complexity: O(1).
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return table_.empty(); }

    /**
     * @brief Reports whether the set is full.
     * @return `true` when `size() == capacity()`.
     * @note Complexity: O(1).
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return table_.full(); }

    /**
     * @brief Returns an iterator to the first occupied slot.
     * @return Iterator to the first stored key, or `end()` when empty.
     * @note Complexity: O(N) worst-case.
     */
    iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(table_.cbegin()); }

    /**
     * @brief Returns the sentinel iterator.
     * @return Iterator past the last key.
     * @note Complexity: O(1).
     */
    iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(table_.cend()); }

    /**
     * @brief Returns a const iterator to the first occupied slot.
     * @return Iterator to the first stored key, or `cend()` when empty.
     * @note Complexity: O(N) worst-case.
     */
    iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns the const sentinel iterator.
     * @return Iterator past the last key.
     * @note Complexity: O(1).
     */
    iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /**
     * @brief Inserts a key when it is not already present.
     * @param key Key to insert.
     * @return `status::ok` on success, `status::already_exists` when the key is
     * already present, or `status::full` when the set has no free slot.
     * @note Complexity: O(N) worst-case.
     */
    status insert(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return table_.insert(key, detail::hash_set_value());
    }

    /**
     * @brief Tests whether a key is present.
     * @param key Key to search for.
     * @return `true` when the key exists, otherwise `false`.
     * @note Complexity: O(N) worst-case.
     */
    bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return table_.contains(key);
    }

    /**
     * @brief Finds a key.
     * @param key Key to search for.
     * @return Iterator to the matching key, or `end()` when absent.
     * @note Complexity: O(N) worst-case.
     */
    iterator find(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(table_.find(key));
    }

    /**
     * @brief Erases a key.
     * @param key Key to erase.
     * @return `status::ok` when a key was removed or `status::not_found` when absent.
     * @note Complexity: O(N) worst-case.
     * @warning Iterators and references to the erased key become invalid.
     */
    status erase(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return table_.erase(key);
    }

    /**
     * @brief Removes all keys from the set.
     * @note Complexity: O(N).
     * @warning All iterators and references are invalidated.
     */
    void clear() CASTLE_NOEXCEPT
    {
        table_.clear();
    }

private:
    table_type table_;
};

}
}

#endif
