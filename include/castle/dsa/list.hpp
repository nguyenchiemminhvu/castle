// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file list.hpp
 * @brief Fixed-capacity doubly linked list (deque-like) backed by an internal node pool.
 *
 * Use this container when elements must be inserted or removed at both ends, or at a known
 * position, in O(1) without dynamic allocation and without invalidating other elements. The list
 * preallocates `N` nodes inside the object and hands them out from a free list. Because every
 * node is linked in both directions, it also supports O(1) repositioning of an existing element
 * (`move_to_front()`, `move_to_back()`, `splice()`), which is the primitive needed to build an
 * LRU cache: keep the most recently used entry at the front, evict from the back, and index the
 * nodes through a `hash_map` of iterators.
 *
 * @code
 * #include "castle/dsa/list.hpp"
 *
 * castle::dsa::list<int, 4U> values{1, 2, 3};
 * values.push_front(0);                 // 0, 1, 2, 3
 * values.move_to_front(--values.end()); // 3, 0, 1, 2
 * values.pop_back();                    // 3, 0, 1
 * @endcode
 *
 * @note Nodes never move while linked into the list; iterators and references stay valid until
 * that specific element is erased or the list is cleared.
 * @warning Capacity is fixed at compile time; insertion reports `status::full` (or returns
 * `end()` for the iterator-returning insert family) when the node pool is exhausted.
 */
#ifndef CASTLE_DSA_LIST_HPP
#define CASTLE_DSA_LIST_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/dsa/initializer_list.hpp"
#include "castle/error/status.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/iterator/tags.hpp"
#include "castle/iterator/reverse_iterator.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"

namespace castle
{
namespace dsa
{

/**
 * @brief Fixed-capacity doubly linked list with an internal free-node pool.
 * @tparam T Element type.
 * @tparam N Maximum number of live nodes.
 * @note All storage is embedded in the list object. The list is circular around an internal
 * sentinel node, so `end()` is a real, decrementable position and reverse iteration works.
 * @warning The container is non-copyable and non-movable because node storage is owned in place.
 * Iterators passed to member functions must belong to the same list object.
 */
template <typename T, size_type N>
class list
{
    static_assert(N > 0U, "list requires a positive capacity");

    struct node_base
    {
        node_base() CASTLE_NOEXCEPT
            : next(nullptr), prev(nullptr) {}
        node_base* next;
        node_base* prev;
    };

    struct node : node_base
    {
        node()
            : node_base(), value_storage() {}
        memory::static_storage<T, 1U> value_storage;
    };

public:
    using value_type = T;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    /** @brief Compact storage type for the element count. */
    using index_type = meta::smallest_uint_t<N>;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;

    class const_iterator;

    /**
     * @brief Bidirectional iterator over mutable list elements.
     * @note Increment and decrement are O(1). Incrementing `end()` or decrementing `begin()`
     * wraps through the internal sentinel and must not be dereferenced.
     */
    class iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = castle::difference_type;
        using pointer = T*;
        using reference = T&;

        /**
         * @brief Constructs a singular iterator that refers to no list.
         * @warning A singular iterator may only be assigned to or compared.
         */
        iterator() CASTLE_NOEXCEPT : node_(nullptr) {}

        /**
         * @brief Dereferences the current element.
         * @return Reference to the stored value.
         * @warning The iterator must not equal `end()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return *value_ptr(static_cast<node*>(node_));
        }

        /**
         * @brief Returns the address of the current element.
         * @return Pointer to the stored value.
         * @warning The iterator must not equal `end()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return value_ptr(static_cast<node*>(node_));
        }

        /**
         * @brief Advances to the next element.
         * @return Reference to `*this`.
         */
        iterator& operator++() CASTLE_NOEXCEPT
        {
            node_ = node_->next;
            return *this;
        }

        /**
         * @brief Advances to the next element and returns the previous position.
         * @return Iterator state before incrementing.
         */
        iterator operator++(int) CASTLE_NOEXCEPT
        {
            iterator temp(*this);
            node_ = node_->next;
            return temp;
        }

        /**
         * @brief Moves to the previous element.
         * @return Reference to `*this`.
         */
        iterator& operator--() CASTLE_NOEXCEPT
        {
            node_ = node_->prev;
            return *this;
        }

        /**
         * @brief Moves to the previous element and returns the previous position.
         * @return Iterator state before decrementing.
         */
        iterator operator--(int) CASTLE_NOEXCEPT
        {
            iterator temp(*this);
            node_ = node_->prev;
            return temp;
        }

        /**
         * @brief Tests whether two iterators reference the same node.
         * @param lhs Left iterator.
         * @param rhs Right iterator.
         * @return `true` when both iterators reference the same node.
         */
        friend bool operator==(CASTLE_CONST iterator& lhs, CASTLE_CONST iterator& rhs) CASTLE_NOEXCEPT
        {
            return lhs.node_ == rhs.node_;
        }

        /**
         * @brief Tests whether two iterators reference different nodes.
         * @param lhs Left iterator.
         * @param rhs Right iterator.
         * @return `true` when the iterators differ.
         */
        friend bool operator!=(CASTLE_CONST iterator& lhs, CASTLE_CONST iterator& rhs) CASTLE_NOEXCEPT
        {
            return lhs.node_ != rhs.node_;
        }

    private:
        friend class list;
        friend class const_iterator;

        explicit iterator(node_base* node_value) CASTLE_NOEXCEPT : node_(node_value) {}

        static T* value_ptr(node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<T>(item->value_storage.address(0U));
        }

        node_base* node_;
    };

    /**
     * @brief Bidirectional iterator over constant list elements.
     * @note Implicitly constructible from `iterator`; mixed comparisons are supported.
     */
    class const_iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST T*;
        using reference = CASTLE_CONST T&;

        /**
         * @brief Constructs a singular iterator that refers to no list.
         * @warning A singular iterator may only be assigned to or compared.
         */
        const_iterator() CASTLE_NOEXCEPT : node_(nullptr) {}

        /**
         * @brief Constructs a const iterator from a mutable iterator.
         * @param other Mutable iterator to copy.
         */
        const_iterator(CASTLE_CONST iterator& other) CASTLE_NOEXCEPT : node_(other.node_) {}

        /**
         * @brief Dereferences the current element.
         * @return Const reference to the stored value.
         * @warning The iterator must not equal `end()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return *value_ptr(static_cast<CASTLE_CONST node*>(node_));
        }

        /**
         * @brief Returns the address of the current element.
         * @return Const pointer to the stored value.
         * @warning The iterator must not equal `end()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return value_ptr(static_cast<CASTLE_CONST node*>(node_));
        }

        /**
         * @brief Advances to the next element.
         * @return Reference to `*this`.
         */
        const_iterator& operator++() CASTLE_NOEXCEPT
        {
            node_ = node_->next;
            return *this;
        }

        /**
         * @brief Advances to the next element and returns the previous position.
         * @return Iterator state before incrementing.
         */
        const_iterator operator++(int) CASTLE_NOEXCEPT
        {
            const_iterator temp(*this);
            node_ = node_->next;
            return temp;
        }

        /**
         * @brief Moves to the previous element.
         * @return Reference to `*this`.
         */
        const_iterator& operator--() CASTLE_NOEXCEPT
        {
            node_ = node_->prev;
            return *this;
        }

        /**
         * @brief Moves to the previous element and returns the previous position.
         * @return Iterator state before decrementing.
         */
        const_iterator operator--(int) CASTLE_NOEXCEPT
        {
            const_iterator temp(*this);
            node_ = node_->prev;
            return temp;
        }

        /**
         * @brief Tests whether two const iterators reference the same node.
         * @param lhs Left iterator.
         * @param rhs Right iterator.
         * @return `true` when both iterators reference the same node.
         */
        friend bool operator==(CASTLE_CONST const_iterator& lhs, CASTLE_CONST const_iterator& rhs) CASTLE_NOEXCEPT
        {
            return lhs.node_ == rhs.node_;
        }

        /**
         * @brief Tests whether two const iterators reference different nodes.
         * @param lhs Left iterator.
         * @param rhs Right iterator.
         * @return `true` when the iterators differ.
         */
        friend bool operator!=(CASTLE_CONST const_iterator& lhs, CASTLE_CONST const_iterator& rhs) CASTLE_NOEXCEPT
        {
            return lhs.node_ != rhs.node_;
        }

    private:
        friend class list;

        explicit const_iterator(CASTLE_CONST node_base* node_value) CASTLE_NOEXCEPT : node_(node_value) {}

        static pointer value_ptr(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<CASTLE_CONST T>(item->value_storage.address(0U));
        }

        CASTLE_CONST node_base* node_;
    };

    using reverse_iterator = castle::reverse_iterator<iterator>;
    using const_reverse_iterator = castle::reverse_iterator<const_iterator>;

    /**
     * @brief Constructs an empty list and initializes the free-node pool.
     * @note Complexity is O(N) because the pool links are initialized.
     */
    list() CASTLE_NOEXCEPT : size_(0U), free_head_(nullptr), sentinel_()
    {
        initialise_pool();
    }

    /**
     * @brief Constructs the list from a brace-initializer list in source order.
     * @param values Source elements.
     * @note Complexity is O(N + values.size()). Elements beyond the capacity are ignored.
     */
    list(initializer_list<T> values) CASTLE_NOEXCEPT : size_(0U), free_head_(nullptr), sentinel_()
    {
        initialise_pool();
        for (CASTLE_CONST T& value : values)
        {
            if (full())
            {
                break;
            }
            emplace_before(&sentinel_, value);
        }
    }

    /**
     * @brief Destroys all active elements.
     * @note Complexity is O(size()).
     */
    ~list() CASTLE_NOEXCEPT { clear(); }

    list(CASTLE_CONST list&) CASTLE_DELETE;
    list& operator=(CASTLE_CONST list&) CASTLE_DELETE;
    list(list&&) CASTLE_DELETE;
    list& operator=(list&&) CASTLE_DELETE;

    /**
     * @brief Returns the fixed compile-time capacity.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the maximum number of storable elements.
     * @return `N`.
     */
    size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the current element count.
     * @return Number of live nodes.
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the number of free nodes remaining in the pool.
     * @return `capacity() - size()`.
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    /**
     * @brief Tests whether the list has no elements.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Tests whether the node pool is exhausted.
     * @return `true` when `size() == capacity()`.
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Returns an iterator to the first element.
     * @return Iterator to the front element, or `end()` when empty.
     */
    iterator begin() CASTLE_NOEXCEPT { return iterator(sentinel_.next); }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const iterator to the front element, or `end()` when empty.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(sentinel_.next); }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const iterator equal to `begin()`.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns the past-the-end iterator.
     * @return Decrementable iterator one position after the last element.
     */
    iterator end() CASTLE_NOEXCEPT { return iterator(&sentinel_); }

    /**
     * @brief Returns the const past-the-end iterator.
     * @return Decrementable const iterator one position after the last element.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(&sentinel_); }

    /**
     * @brief Returns the const past-the-end iterator.
     * @return Const iterator equal to `end()`.
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /**
     * @brief Returns a reverse iterator to the last element.
     * @return Reverse iterator starting at the back of the list.
     */
    reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(end()); }

    /**
     * @brief Returns a const reverse iterator to the last element.
     * @return Const reverse iterator starting at the back of the list.
     */
    const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    /**
     * @brief Returns a const reverse iterator to the last element.
     * @return Const reverse iterator equal to `rbegin()`.
     */
    const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return rbegin(); }

    /**
     * @brief Returns the reverse past-the-end iterator.
     * @return Reverse iterator one position before the first element.
     */
    reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(begin()); }

    /**
     * @brief Returns the const reverse past-the-end iterator.
     * @return Const reverse iterator one position before the first element.
     */
    const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    /**
     * @brief Returns the const reverse past-the-end iterator.
     * @return Const reverse iterator equal to `rend()`.
     */
    const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return rend(); }

    /**
     * @brief Returns the first element.
     * @return Reference to the front value.
     * @warning The list must not be empty.
     */
    reference front() CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Returns the first element.
     * @return Const reference to the front value.
     * @warning The list must not be empty.
     */
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Returns the last element.
     * @return Reference to the back value.
     * @warning The list must not be empty.
     */
    reference back() CASTLE_NOEXCEPT { return *iterator(sentinel_.prev); }

    /**
     * @brief Returns the last element.
     * @return Const reference to the back value.
     * @warning The list must not be empty.
     */
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return *const_iterator(sentinel_.prev); }

    /**
     * @brief Inserts a copy of `value` at the front.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    status push_front(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return emplace_front(value);
    }

    /**
     * @brief Inserts `value` at the front by move construction.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    status push_front(T&& value) CASTLE_NOEXCEPT
    {
        return emplace_front(CASTLE_MOVE(value));
    }

    /**
     * @brief Constructs a new front element in place.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to `T`.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    template <typename... Args>
    status emplace_front(Args&&... args) CASTLE_NOEXCEPT
    {
        return to_status(emplace_before(sentinel_.next, CASTLE_FORWARD<Args>(args)...));
    }

    /**
     * @brief Inserts a copy of `value` at the back.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    status push_back(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return emplace_back(value);
    }

    /**
     * @brief Inserts `value` at the back by move construction.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    status push_back(T&& value) CASTLE_NOEXCEPT
    {
        return emplace_back(CASTLE_MOVE(value));
    }

    /**
     * @brief Constructs a new back element in place.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to `T`.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    template <typename... Args>
    status emplace_back(Args&&... args) CASTLE_NOEXCEPT
    {
        return to_status(emplace_before(&sentinel_, CASTLE_FORWARD<Args>(args)...));
    }

    /**
     * @brief Removes the first element.
     * @return `status::ok` on success or `status::empty` when the list has no elements.
     * @note Complexity is O(1).
     */
    status pop_front() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        erase_node(static_cast<node*>(sentinel_.next));
        return status::ok;
    }

    /**
     * @brief Removes the last element.
     * @return `status::ok` on success or `status::empty` when the list has no elements.
     * @note Complexity is O(1).
     */
    status pop_back() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        erase_node(static_cast<node*>(sentinel_.prev));
        return status::ok;
    }

    /**
     * @brief Constructs an element immediately before `position`.
     * @tparam Args Constructor argument types.
     * @param position Insertion point; `end()` appends.
     * @param args Arguments forwarded to `T`.
     * @return Iterator to the new element, or `end()` when no free node remains. A successful
     * insertion never returns `end()`.
     * @note Complexity is O(1).
     */
    template <typename... Args>
    iterator emplace(const_iterator position, Args&&... args) CASTLE_NOEXCEPT
    {
        node* item = emplace_before(mutable_node(position), CASTLE_FORWARD<Args>(args)...);
        return (item == nullptr) ? end() : iterator(item);
    }

    /**
     * @brief Inserts a copy of `value` immediately before `position`.
     * @param position Insertion point; `end()` appends.
     * @param value Element to insert.
     * @return Iterator to the new element, or `end()` when no free node remains.
     * @note Complexity is O(1).
     */
    iterator insert(const_iterator position, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return emplace(position, value);
    }

    /**
     * @brief Inserts `value` immediately before `position` by move construction.
     * @param position Insertion point; `end()` appends.
     * @param value Element to insert.
     * @return Iterator to the new element, or `end()` when no free node remains.
     * @note Complexity is O(1).
     */
    iterator insert(const_iterator position, T&& value) CASTLE_NOEXCEPT
    {
        return emplace(position, CASTLE_MOVE(value));
    }

    /**
     * @brief Erases the element at `position`.
     * @param position Element to erase. Erasing `end()` is a no-op.
     * @return Iterator to the element that followed the erased one, or `end()`.
     * @note Complexity is O(1). Only iterators to the erased element are invalidated.
     */
    iterator erase(const_iterator position) CASTLE_NOEXCEPT
    {
        node_base* item = mutable_node(position);
        if (item == &sentinel_)
        {
            return end();
        }
        node_base* following = item->next;
        erase_node(static_cast<node*>(item));
        return iterator(following);
    }

    /**
     * @brief Erases the elements in the range `[first, last)`.
     * @param first First element to erase.
     * @param last One past the last element to erase.
     * @return Iterator equal to `last`.
     * @note Complexity is O(distance(first, last)).
     */
    iterator erase(const_iterator first, const_iterator last) CASTLE_NOEXCEPT
    {
        node_base* current = mutable_node(first);
        node_base* stop = mutable_node(last);
        while (current != stop)
        {
            node_base* following = current->next;
            erase_node(static_cast<node*>(current));
            current = following;
        }
        return iterator(stop);
    }

    /**
     * @brief Moves `element` to sit immediately before `position` without copying the value.
     * @param position Destination; `end()` moves to the back.
     * @param element Element of this list to relocate. `end()` is a no-op.
     * @note Complexity is O(1). No node is allocated and no iterator is invalidated; `element`
     * keeps referring to the same value at its new place.
     */
    void splice(const_iterator position, const_iterator element) CASTLE_NOEXCEPT
    {
        node_base* at = mutable_node(position);
        node_base* item = mutable_node(element);
        if ((item == &sentinel_) || (item == at) || (item->next == at))
        {
            return;
        }
        unlink(item);
        link_before(at, item);
    }

    /**
     * @brief Makes `element` the first element (most-recently-used position for an LRU).
     * @param element Element of this list to relocate.
     * @note Complexity is O(1); equivalent to `splice(begin(), element)`.
     */
    void move_to_front(const_iterator element) CASTLE_NOEXCEPT
    {
        splice(begin(), element);
    }

    /**
     * @brief Makes `element` the last element.
     * @param element Element of this list to relocate.
     * @note Complexity is O(1); equivalent to `splice(end(), element)`.
     */
    void move_to_back(const_iterator element) CASTLE_NOEXCEPT
    {
        splice(end(), element);
    }

    /**
     * @brief Removes every element equal to `value`.
     * @param value Value to remove.
     * @return Number of removed elements.
     * @note Complexity is O(size()).
     */
    size_type remove(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        size_type removed = 0U;
        node_base* current = sentinel_.next;
        while (current != &sentinel_)
        {
            node* item = static_cast<node*>(current);
            current = current->next;
            if (*value_ptr(item) == value)
            {
                erase_node(item);
                ++removed;
            }
        }
        return removed;
    }

    /**
     * @brief Reverses the element order in place.
     * @note Complexity is O(size()). No value is copied or moved; iterators keep referring to
     * the same elements.
     */
    void reverse() CASTLE_NOEXCEPT
    {
        node_base* current = &sentinel_;
        do
        {
            node_base* following = current->next;
            current->next = current->prev;
            current->prev = following;
            current = following;
        } while (current != &sentinel_);
    }

    /**
     * @brief Destroys every element and returns all nodes to the free pool.
     * @note Complexity is O(size()).
     */
    void clear() CASTLE_NOEXCEPT
    {
        node_base* current = sentinel_.next;
        while (current != &sentinel_)
        {
            node* item = static_cast<node*>(current);
            current = current->next;
            pool_node(item);
        }
        sentinel_.next = &sentinel_;
        sentinel_.prev = &sentinel_;
        size_ = 0U;
    }

    /**
     * @brief Finds the first element equal to `value`.
     * @param value Value to search for.
     * @return Iterator to the first matching element, or `end()` when not found.
     * @note Complexity is O(size()).
     */
    iterator find(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        iterator it = begin();
        while ((it != end()) && !(*it == value))
        {
            ++it;
        }
        return it;
    }

    /**
     * @brief Finds the first element equal to `value`.
     * @param value Value to search for.
     * @return Const iterator to the first matching element, or `end()` when not found.
     * @note Complexity is O(size()).
     */
    const_iterator find(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        const_iterator it = begin();
        while ((it != end()) && !(*it == value))
        {
            ++it;
        }
        return it;
    }

private:
    static T* value_ptr(node* item) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<T>(item->value_storage.address(0U));
    }

    /**
     * @brief Recovers the mutable node pointer from a const iterator.
     * @param position Iterator naming a node of this list.
     * @return Node pointer.
     * @note Safe because all nodes live in the non-const `nodes_` array of a mutable list;
     * only mutating members call this and they are unavailable on a const list.
     */
    static node_base* mutable_node(const_iterator position) CASTLE_NOEXCEPT
    {
        return const_cast<node_base*>(position.node_);
    }

    static status to_status(CASTLE_CONST node* item) CASTLE_NOEXCEPT
    {
        return (item == nullptr) ? status::full : status::ok;
    }

    /**
     * @brief Links `item` immediately before `at`.
     * @param at Linked node (or the sentinel) that will follow `item`.
     * @param item Unlinked node to insert.
     */
    static void link_before(node_base* at, node_base* item) CASTLE_NOEXCEPT
    {
        item->prev = at->prev;
        item->next = at;
        at->prev->next = item;
        at->prev = item;
    }

    /**
     * @brief Detaches `item` from its neighbors.
     * @param item Linked node to detach.
     */
    static void unlink(node_base* item) CASTLE_NOEXCEPT
    {
        item->prev->next = item->next;
        item->next->prev = item->prev;
    }

    /**
     * @brief Takes a node from the free pool, constructs a value in it and links it before `at`.
     * @param at Linked node (or the sentinel) that will follow the new element.
     * @param args Arguments forwarded to `T`.
     * @return The new node, or `nullptr` when the pool is exhausted.
     */
    template <typename... Args>
    node* emplace_before(node_base* at, Args&&... args) CASTLE_NOEXCEPT
    {
        if (free_head_ == nullptr)
        {
            return nullptr;
        }
        node* item = static_cast<node*>(free_head_);
        free_head_ = item->next;
        memory::construct_at<T>(item->value_storage.address(0U), CASTLE_FORWARD<Args>(args)...);
        link_before(at, item);
        ++size_;
        return item;
    }

    /**
     * @brief Unlinks a live node, destroys its value and returns it to the free pool.
     * @param item Live node to erase.
     */
    void erase_node(node* item) CASTLE_NOEXCEPT
    {
        unlink(item);
        pool_node(item);
        --size_;
    }

    /**
     * @brief Destroys the value of `item` and pushes the node onto the free list.
     * @param item Node whose value is alive.
     * @note Does not touch the live links or `size_`.
     */
    void pool_node(node* item) CASTLE_NOEXCEPT
    {
        memory::destroy_at(value_ptr(item));
        item->next = free_head_;
        item->prev = nullptr;
        free_head_ = item;
    }

    /**
     * @brief Chains every node into the free list and self-links the sentinel.
     */
    void initialise_pool() CASTLE_NOEXCEPT
    {
        free_head_ = nullptr;
        for (size_type i = N; i > 0U; --i)
        {
            nodes_[i - 1U].next = free_head_;
            free_head_ = &nodes_[i - 1U];
        }
        sentinel_.next = &sentinel_;
        sentinel_.prev = &sentinel_;
    }

    node nodes_[N];
    index_type size_;
    node_base* free_head_;
    node_base sentinel_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_LIST_HPP
