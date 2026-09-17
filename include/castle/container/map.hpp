// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file map.hpp
 * @brief Fixed-capacity ordered map backed by Castle's AVL tree.
 *
 * Use `map<Key, T, N>` when key/value pairs must be stored in sorted key order
 * without heap allocation. Capacity is fixed by `N`, nodes live in internal
 * static storage, and insertion, lookup, bound queries, and erasure are
 * O(log N) worst-case because the underlying AVL tree remains balanced.
 *
 * @code
 * castle::container::map<int, int, 4U> values;
 * values.insert(1, 10);
 * const int* mapped = values.get(1);
 * @endcode
 */
#ifndef CASTLE_CONTAINER_MAP_HPP
#define CASTLE_CONTAINER_MAP_HPP

#include "castle/container/avl_tree.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/core/error_handler.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity ordered map of unique keys to mapped values.
 * @tparam Key Key type stored in sorted order.
 * @tparam T Mapped value type.
 * @tparam N Maximum number of live entries stored without dynamic allocation.
 * @tparam Compare Strict-weak-order comparator used for key ordering.
 * @note Storage is embedded. Capacity/state queries are O(1).
 * @note `insert()`, `try_emplace()`, `find()`, `contains()`, `get()`,
 * `lower_bound()`, `upper_bound()`, and `erase()` are O(log N) worst-case.
 * @warning Erasing an element invalidates iterators and references to that
 * element only. Other iterators remain valid because nodes never move in memory.
 * `clear()` invalidates all iterators and references.
 */
template <typename Key, typename T, size_type N, typename Compare = castle::less<Key>>
class map
{
    using tree_type = avl_tree<Key, T, N, Compare>;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = typename tree_type::value_type;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using iterator = typename tree_type::iterator;
    using const_iterator = typename tree_type::const_iterator;

    /**
     * @brief Constructs an empty map using the default comparator.
     * @note Complexity: O(1).
     */
    map() CASTLE_NOEXCEPT : tree_() {}

    /**
     * @brief Constructs an empty map using a caller-supplied comparator.
     * @param compare Comparator copied into the underlying AVL tree.
     * @note Complexity: O(1).
     */
    explicit map(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT : tree_(compare) {}

    /**
     * @brief Constructs a map from an initializer list.
     * @param list Key/value pairs inserted in list order.
     * @note Complexity: O(list.size() * log N) worst-case.
     * @warning `list.size()` must not exceed `N`; the constructor reports a
     * violated precondition with `CASTLE_ASSERT`. Duplicate keys are ignored.
     */
    map(initializer_list<value_type> list) CASTLE_NOEXCEPT : tree_()
    {
        for (CASTLE_CONST value_type& entry : list)
        {
            if (full())
            {
                break;
            }
            insert(entry);
        }
    }

    /**
     * @brief Destroys the map and releases all constructed nodes.
     * @note Complexity: O(N).
     */
    ~map() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @param other Source map.
     * @warning Maps are non-copyable.
     */
    map(CASTLE_CONST map&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @param other Source map.
     * @return This declaration is deleted.
     * @warning Maps are non-copyable.
     */
    map& operator=(CASTLE_CONST map&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @param other Source map.
     * @warning Maps are non-movable.
     */
    map(map&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @param other Source map.
     * @return This declaration is deleted.
     * @warning Maps are non-movable.
     */
    map& operator=(map&&) CASTLE_DELETE;

    /**
     * @brief Returns the compile-time capacity.
     * @return Maximum number of key/value pairs storable in the map.
     * @note Complexity: O(1).
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the number of stored entries.
     * @return Current map size.
     * @note Complexity: O(1).
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.size(); }

    /**
     * @brief Returns the maximum size.
     * @return `N`.
     * @note Complexity: O(1).
     */
    size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the remaining insertion capacity.
     * @return `capacity() - size()`.
     * @note Complexity: O(1).
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.available(); }

    /**
     * @brief Reports whether the map is empty.
     * @return `true` when no entries are stored.
     * @note Complexity: O(1).
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.empty(); }

    /**
     * @brief Reports whether the map is full.
     * @return `true` when `size() == capacity()`.
     * @note Complexity: O(1).
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.full(); }

    /**
     * @brief Returns a mutable iterator to the smallest key.
     * @return Iterator to the first entry in sorted key order, or `end()` when empty.
     * @note Complexity: O(log N) worst-case.
     */
    iterator begin() CASTLE_NOEXCEPT { return tree_.begin(); }

    /**
     * @brief Returns a const iterator to the smallest key.
     * @return Const iterator to the first entry in sorted key order, or `end()` when empty.
     * @note Complexity: O(log N) worst-case.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.begin(); }

    /**
     * @brief Returns a const iterator to the smallest key.
     * @return Const iterator to the first entry in sorted key order, or `cend()` when empty.
     * @note Complexity: O(log N) worst-case.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.cbegin(); }

    /**
     * @brief Returns the mutable sentinel iterator.
     * @return Iterator past the last entry.
     * @note Complexity: O(1).
     */
    iterator end() CASTLE_NOEXCEPT { return tree_.end(); }

    /**
     * @brief Returns the const sentinel iterator.
     * @return Const iterator past the last entry.
     * @note Complexity: O(1).
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.end(); }

    /**
     * @brief Returns the const sentinel iterator.
     * @return Const iterator past the last entry.
     * @note Complexity: O(1).
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.cend(); }

    /**
     * @brief Inserts a key/value pair from a value object.
     * @param value Pair whose key and mapped value are inserted.
     * @return `status::ok` on success, `status::already_exists` when the key is
     * already present, or `status::full` when the map has no free node.
     * @note Complexity: O(log N) worst-case.
     */
    status insert(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        return tree_.insert(value.first, value.second);
    }

    /**
     * @brief Inserts a key/value pair by copying the mapped value.
     * @param key Key to insert.
     * @param value Mapped value to copy into the map.
     * @return `status::ok` on success, `status::already_exists` when the key is
     * already present, or `status::full` when the map has no free node.
     * @note Complexity: O(log N) worst-case.
     */
    status insert(CASTLE_CONST key_type& key, CASTLE_CONST mapped_type& value) CASTLE_NOEXCEPT
    {
        return tree_.insert(key, value);
    }

    /**
     * @brief Inserts a key/value pair by moving the mapped value.
     * @param key Key to insert.
     * @param value Mapped value to move into the map.
     * @return `status::ok` on success, `status::already_exists` when the key is
     * already present, or `status::full` when the map has no free node.
     * @note Complexity: O(log N) worst-case.
     */
    status insert(CASTLE_CONST key_type& key, mapped_type&& value) CASTLE_NOEXCEPT
    {
        return tree_.insert(key, CASTLE_MOVE(value));
    }

    /**
     * @brief Constructs the mapped value in place when the key is absent.
     * @tparam Args Constructor argument types for `mapped_type`.
     * @param key Key to insert.
     * @param args Arguments forwarded to the mapped-value constructor.
     * @return `status::ok` on success, `status::already_exists` when the key is
     * already present, or `status::full` when the map has no free node.
     * @note Complexity: O(log N) worst-case.
     */
    template <typename... Args>
    status try_emplace(CASTLE_CONST key_type& key, Args&&... args) CASTLE_NOEXCEPT
    {
        return tree_.emplace(key, CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Finds a key.
     * @param key Key to search for.
     * @return Iterator to the matching entry, or `end()` when absent.
     * @note Complexity: O(log N) worst-case.
     */
    iterator find(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.find(key);
    }

    /**
     * @brief Finds a key.
     * @param key Key to search for.
     * @return Const iterator to the matching entry, or `end()` when absent.
     * @note Complexity: O(log N) worst-case.
     */
    const_iterator find(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.find(key);
    }

    /**
     * @brief Tests whether a key is present.
     * @param key Key to search for.
     * @return `true` when the key exists, otherwise `false`.
     * @note Complexity: O(log N) worst-case.
     */
    bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.contains(key);
    }

    /**
     * @brief Returns a mutable pointer to the mapped value for `key`.
     * @param key Key to search for.
     * @return Pointer to the mapped value, or `nullptr` when the key is absent.
     * @note Complexity: O(log N) worst-case.
     */
    mapped_type* get(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.mapped(key);
    }

    /**
     * @brief Returns a const pointer to the mapped value for `key`.
     * @param key Key to search for.
     * @return Pointer to the mapped value, or `nullptr` when the key is absent.
     * @note Complexity: O(log N) worst-case.
     */
    CASTLE_CONST mapped_type* get(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.mapped(key);
    }

    /**
     * @brief Finds the first key that is not less than `key`.
     * @param key Lower-bound search key.
     * @return Iterator to the first entry whose key is `>= key`, or `end()` when none exists.
     * @note Complexity: O(log N) worst-case.
     */
    iterator lower_bound(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.lower_bound(key);
    }

    /**
     * @brief Finds the first key that is not less than `key`.
     * @param key Lower-bound search key.
     * @return Const iterator to the first entry whose key is `>= key`, or `end()` when none exists.
     * @note Complexity: O(log N) worst-case.
     */
    const_iterator lower_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.lower_bound(key);
    }

    /**
     * @brief Finds the first key that is greater than `key`.
     * @param key Upper-bound search key.
     * @return Iterator to the first entry whose key is `> key`, or `end()` when none exists.
     * @note Complexity: O(log N) worst-case.
     */
    iterator upper_bound(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.upper_bound(key);
    }

    /**
     * @brief Finds the first key that is greater than `key`.
     * @param key Upper-bound search key.
     * @return Const iterator to the first entry whose key is `> key`, or `end()` when none exists.
     * @note Complexity: O(log N) worst-case.
     */
    const_iterator upper_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.upper_bound(key);
    }

    /**
     * @brief Erases the entry for `key`.
     * @param key Key to erase.
     * @return `status::ok` when an entry was removed or `status::not_found` when absent.
     * @note Complexity: O(log N) worst-case.
     * @warning Iterators and references to the erased entry become invalid.
     */
    status erase(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.erase(key);
    }

    /**
     * @brief Erases the entry at `position`.
     * @param position Iterator identifying the entry to erase.
     * @return Iterator to the entry that followed the erased one, or `end()`.
     * @note Complexity: O(log N) worst-case.
     * @warning `position` must either be a valid iterator from this map or `end()`.
     */
    iterator erase(iterator position) CASTLE_NOEXCEPT
    {
        return tree_.erase(position);
    }

    /**
     * @brief Removes all entries from the map.
     * @note Complexity: O(N).
     * @warning All iterators and references are invalidated.
     */
    void clear() CASTLE_NOEXCEPT
    {
        tree_.clear();
    }

private:
    tree_type tree_;
};

}
}

#endif
