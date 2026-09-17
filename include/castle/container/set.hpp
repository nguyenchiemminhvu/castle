// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file set.hpp
 * @brief Fixed-capacity ordered set backed by Castle's AVL tree.
 *
 * Use `set<Key, N>` when unique keys must be stored in sorted order without
 * heap allocation. Capacity is fixed by `N`, nodes live in static internal
 * storage, and insertion, lookup, bound queries, and erasure are O(log N)
 * worst-case because the underlying AVL tree remains balanced.
 *
 * @code
 * castle::container::set<int, 4U> ids;
 * ids.insert(3);
 * bool present = ids.contains(3);
 * @endcode
 */
#ifndef CASTLE_CONTAINER_SET_HPP
#define CASTLE_CONTAINER_SET_HPP

#include "castle/container/avl_tree.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/core/error_handler.hpp"

namespace castle
{
namespace container
{
namespace detail
{
struct set_value
{
};
}

/**
 * @brief Fixed-capacity ordered set of unique keys.
 * @tparam Key Key type stored in sorted order.
 * @tparam N Maximum number of live keys stored without dynamic allocation.
 * @tparam Compare Strict-weak-order comparator used for key ordering.
 * @note Storage is embedded. Capacity/state queries are O(1).
 * @note `insert()`, `find()`, `contains()`, `lower_bound()`, `upper_bound()`,
 * and `erase()` are O(log N) worst-case.
 * @warning Erasing a key invalidates only iterators and references to that
 * key. Other iterators remain valid because nodes never move in memory.
 * `clear()` invalidates all iterators.
 */
template <typename Key, size_type N, typename Compare = castle::less<Key>>
class set
{
    using tree_type = avl_tree<Key, detail::set_value, N, Compare>;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;

    /**
     * @brief Bidirectional iterator over set keys.
     * @note Dereferencing yields a const key reference because keys are immutable.
     * @warning Dereferencing `end()`, incrementing `end()`, or decrementing
     * `begin()` is undefined.
     */
    class const_iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
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
         * @brief Wraps the underlying AVL-tree iterator.
         * @param it Underlying iterator to wrap.
         * @note Complexity: O(1).
         */
        explicit const_iterator(typename tree_type::const_iterator it) CASTLE_NOEXCEPT : it_(it) {}

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
         * @brief Advances to the next key in sorted order.
         * @return Reference to `*this`.
         * @note Complexity: O(log N) worst-case.
         */
        const_iterator& operator++() CASTLE_NOEXCEPT { ++it_; return *this; }

        /**
         * @brief Advances to the next key in sorted order and returns the previous iterator.
         * @return Iterator state before incrementing.
         * @note Complexity: O(log N) worst-case.
         */
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }

        /**
         * @brief Moves to the previous key in sorted order.
         * @return Reference to `*this`.
         * @note Complexity: O(log N) worst-case.
         */
        const_iterator& operator--() CASTLE_NOEXCEPT { --it_; return *this; }

        /**
         * @brief Moves to the previous key in sorted order and returns the previous iterator.
         * @return Iterator state before decrementing.
         * @note Complexity: O(log N) worst-case.
         */
        const_iterator operator--(int) CASTLE_NOEXCEPT { const_iterator temp(*this); --(*this); return temp; }

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
        typename tree_type::const_iterator it_;
        friend class set;
    };

    using iterator = const_iterator;

    /**
     * @brief Constructs an empty set using the default comparator.
     * @note Complexity: O(1).
     */
    set() CASTLE_NOEXCEPT : tree_() {}

    /**
     * @brief Constructs an empty set using a caller-supplied comparator.
     * @param compare Comparator copied into the underlying AVL tree.
     * @note Complexity: O(1).
     */
    explicit set(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT : tree_(compare) {}

    /**
     * @brief Constructs a set from an initializer list.
     * @param list Keys inserted in list order.
     * @note Complexity: O(list.size() * log N) worst-case.
     * @warning `list.size()` must not exceed `N`; the constructor reports a
     * violated precondition with `CASTLE_ASSERT`. Duplicate keys are ignored.
     */
    set(initializer_list<key_type> list) CASTLE_NOEXCEPT : tree_()
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
     * @brief Destroys the set and releases all constructed nodes.
     * @note Complexity: O(N).
     */
    ~set() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @param other Source set.
     * @warning Sets are non-copyable.
     */
    set(CASTLE_CONST set&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @param other Source set.
     * @return This declaration is deleted.
     * @warning Sets are non-copyable.
     */
    set& operator=(CASTLE_CONST set&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @param other Source set.
     * @warning Sets are non-movable.
     */
    set(set&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @param other Source set.
     * @return This declaration is deleted.
     * @warning Sets are non-movable.
     */
    set& operator=(set&&) CASTLE_DELETE;

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
     * @brief Reports whether the set is empty.
     * @return `true` when no keys are stored.
     * @note Complexity: O(1).
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.empty(); }

    /**
     * @brief Reports whether the set is full.
     * @return `true` when `size() == capacity()`.
     * @note Complexity: O(1).
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.full(); }

    /**
     * @brief Returns an iterator to the smallest key.
     * @return Iterator to the first key in sorted order, or `end()` when empty.
     * @note Complexity: O(log N) worst-case.
     */
    iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(tree_.cbegin()); }

    /**
     * @brief Returns the sentinel iterator.
     * @return Iterator past the last key.
     * @note Complexity: O(1).
     */
    iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(tree_.cend()); }

    /**
     * @brief Returns a const iterator to the smallest key.
     * @return Iterator to the first key in sorted order, or `cend()` when empty.
     * @note Complexity: O(log N) worst-case.
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
     * already present, or `status::full` when the set has no free node.
     * @note Complexity: O(log N) worst-case.
     */
    status insert(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        detail::set_value value;
        return tree_.insert(key, value);
    }

    /**
     * @brief Finds a key.
     * @param key Key to search for.
     * @return Iterator to the matching key, or `end()` when absent.
     * @note Complexity: O(log N) worst-case.
     */
    iterator find(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(tree_.find(key));
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
     * @brief Finds the first key that is not less than `key`.
     * @param key Lower-bound search key.
     * @return Iterator to the first key `>= key`, or `end()` when none exists.
     * @note Complexity: O(log N) worst-case.
     */
    iterator lower_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(tree_.lower_bound(key));
    }

    /**
     * @brief Finds the first key that is greater than `key`.
     * @param key Upper-bound search key.
     * @return Iterator to the first key `> key`, or `end()` when none exists.
     * @note Complexity: O(log N) worst-case.
     */
    iterator upper_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(tree_.upper_bound(key));
    }

    /**
     * @brief Erases a key.
     * @param key Key to erase.
     * @return `status::ok` when a key was removed or `status::not_found` when absent.
     * @note Complexity: O(log N) worst-case.
     * @warning Iterators and references to the erased key become invalid.
     */
    status erase(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.erase(key);
    }

    /**
     * @brief Removes all keys from the set.
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
