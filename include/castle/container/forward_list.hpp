// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file forward_list.hpp
 * @brief Fixed-capacity singly linked list backed by an internal node pool.
 *
 * Use this container when stable node addresses and O(1) insertion/erasure after a known
 * position are needed without dynamic allocation. The list preallocates `N` nodes inside the
 * object, allocates nodes from a free list, and performs linear-time search, removal-by-value,
 * and reversal. Front insertion/removal and `erase_after()` are O(1).
 *
 * @note Nodes never move while linked into the list.
 * @warning Capacity is fixed at compile time; insertion returns `status::full` when the node
 * pool is exhausted.
 */
#ifndef CASTLE_CONTAINER_FORWARD_LIST_HPP
#define CASTLE_CONTAINER_FORWARD_LIST_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/error/status.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/iterator/tags.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity singly linked list with an internal free-node pool.
 * @tparam T Element type.
 * @tparam N Maximum number of live nodes.
 * @note All storage is embedded in the list object. Iterators and references to a node remain
 * valid until that specific node is erased or the list is cleared.
 * @warning The container is non-copyable and non-movable because node storage is owned in place.
 */
template <typename T, size_type N>
class forward_list
{
    static_assert(N > 0U, "forward_list requires a positive capacity");

    struct node_base
    {
        node_base() CASTLE_NOEXCEPT
            : next(nullptr) {}
        node_base* next;
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
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;

    class const_iterator;

    /**
     * @brief Forward iterator over mutable list elements.
     * @note Increment is O(1). Dereferencing requires that the iterator does not equal `end()`.
     */
    class iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = T;
        using difference_type = castle::difference_type;
        using pointer = T*;
        using reference = T&;

        /**
         * @brief Constructs an end iterator.
         */
        iterator() CASTLE_NOEXCEPT : node_(nullptr) {}

        /**
         * @brief Constructs an iterator from an internal node pointer.
         * @param node_value Node addressed by the iterator.
         */
        explicit iterator(node_base* node_value) CASTLE_NOEXCEPT : node_(node_value) {}

        /**
         * @brief Dereferences the current element.
         * @return Reference to the stored value.
         * @warning The iterator must not equal `end()` or `before_begin()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *value_ptr(static_cast<node*>(node_)); }

        /**
         * @brief Returns the address of the current element.
         * @return Pointer to the stored value.
         * @warning The iterator must not equal `end()` or `before_begin()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return value_ptr(static_cast<node*>(node_)); }

        /**
         * @brief Advances to the next node.
         * @return Reference to `*this`.
         * @note Complexity is O(1).
         */
        iterator& operator++() CASTLE_NOEXCEPT { if (node_ != nullptr) node_ = node_->next; return *this; }

        /**
         * @brief Returns the current iterator and then advances to the next node.
         * @return Iterator state before incrementing.
         */
        iterator operator++(int) CASTLE_NOEXCEPT { iterator temp(*this); ++(*this); return temp; }

        /**
         * @brief Tests whether two iterators reference the same node.
         * @param other Iterator to compare with.
         * @return `true` when both iterators reference the same node.
         */
        bool operator==(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return node_ == other.node_; }

        /**
         * @brief Tests whether two iterators reference different nodes.
         * @param other Iterator to compare with.
         * @return `true` when the iterators differ.
         */
        bool operator!=(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

        /**
         * @brief Converts to a const iterator.
         * @return Const iterator referencing the same node.
         */
        operator const_iterator() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(node_); }

    private:
        friend class forward_list;
        friend class const_iterator;

        /**
         * @brief Retrieves a pointer to the stored value from the given node.
         * @param item Node from which to retrieve the stored value.
         * @return Pointer to the stored value.
         */
        static T* value_ptr(node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<T>(item->value_storage.address(0U));
        }
        node_base* node_;
    };

    /**
     * @brief Forward iterator over constant list elements.
     * @note Increment is O(1). Dereferencing requires that the iterator does not equal `end()`.
     */
    class const_iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = T;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST T*;
        using reference = CASTLE_CONST T&;

        /**
         * @brief Constructs an end iterator.
         */
        const_iterator() CASTLE_NOEXCEPT : node_(nullptr) {}

        /**
         * @brief Constructs an iterator from an internal node pointer.
         * @param node_value Node addressed by the iterator.
         */
        explicit const_iterator(CASTLE_CONST node_base* node_value) CASTLE_NOEXCEPT : node_(node_value) {}

        /**
         * @brief Constructs a const iterator from a mutable iterator.
         * @param other Mutable iterator to copy.
         */
        const_iterator(CASTLE_CONST iterator& other) CASTLE_NOEXCEPT : node_(other.node_) {}

        /**
         * @brief Dereferences the current element.
         * @return Const reference to the stored value.
         * @warning The iterator must not equal `end()` or `before_begin()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *value_ptr(static_cast<CASTLE_CONST node*>(node_)); }

        /**
         * @brief Returns the address of the current element.
         * @return Const pointer to the stored value.
         * @warning The iterator must not equal `end()` or `before_begin()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return value_ptr(static_cast<CASTLE_CONST node*>(node_)); }

        /**
         * @brief Advances to the next node.
         * @return Reference to `*this`.
         */
        const_iterator& operator++() CASTLE_NOEXCEPT { if (node_ != nullptr) node_ = node_->next; return *this; } // LCOV_EXCL_BR_LINE

        /**
         * @brief Returns the current iterator and then advances to the next node.
         * @return Iterator state before incrementing.
         */
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }

        /**
         * @brief Tests whether two const iterators reference the same node.
         * @param other Iterator to compare with.
         * @return `true` when both iterators reference the same node.
         */
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return node_ == other.node_; }

        /**
         * @brief Tests whether two const iterators reference different nodes.
         * @param other Iterator to compare with.
         * @return `true` when the iterators differ.
         */
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

    private:
        friend class forward_list;

        /**
         * @brief Retrieves a pointer to the stored value from the given node.
         * @param item Node from which to retrieve the stored value.
         * @return Pointer to the stored value.
         */
        static pointer value_ptr(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<CASTLE_CONST T>(item->value_storage.address(0U));
        }
        CASTLE_CONST node_base* node_;
    };

    /**
     * @brief Constructs an empty list and initializes the free-node pool.
     * @note Complexity is O(N) because the pool links are initialized.
     */
    forward_list() CASTLE_NOEXCEPT : head_(nullptr), size_(0U), free_head_(nullptr), before_()
    {
        initialise_pool();
    }

    /**
     * @brief Constructs the list from a brace-initializer list in source order.
     * @param list Source elements.
     * @note Complexity is O(N + list.size()) because the pool is initialized and each element is
     * appended after the current tail.
     * @warning Oversized initializer lists trigger `CASTLE_ASSERT`.
     */
    forward_list(initializer_list<T> list) CASTLE_NOEXCEPT
        : head_(nullptr), size_(0U), free_head_(nullptr), before_()
    {
        initialise_pool();
        iterator tail = before_begin();
        for (CASTLE_CONST T& value : list)
        {
            if (full())
            {
                break;
            }
            emplace_after(tail, value);
            ++tail;
        }
    }

    /**
     * @brief Destroys all active nodes.
     * @note Complexity is O(size()).
     */
    ~forward_list() CASTLE_NOEXCEPT { clear(); }

    forward_list(CASTLE_CONST forward_list&) CASTLE_DELETE;
    forward_list& operator=(CASTLE_CONST forward_list&) CASTLE_DELETE;
    forward_list(forward_list&&) CASTLE_DELETE;
    forward_list& operator=(forward_list&&) CASTLE_DELETE;

    /**
     * @brief Returns the fixed compile-time capacity.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the current element count.
     * @return Number of live nodes.
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the maximum number of storable elements.
     * @return `N`.
     */
    size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

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
     * @return Iterator to the head node or `end()` when empty.
     */
    iterator begin() CASTLE_NOEXCEPT { return iterator(head_); }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const iterator to the head node or `end()` when empty.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(head_); }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const iterator equal to `begin()`.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns the end iterator.
     * @return Iterator that compares equal to the past-the-end position.
     */
    iterator end() CASTLE_NOEXCEPT { return iterator(nullptr); }

    /**
     * @brief Returns the const end iterator.
     * @return Const iterator that compares equal to the past-the-end position.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(nullptr); }

    /**
     * @brief Returns the const end iterator.
     * @return Const iterator equal to `end()`.
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /**
     * @brief Returns an iterator to the sentinel before the first element.
     * @return Iterator suitable for `emplace_after()` and `erase_after()` at the list head.
     */
    iterator before_begin() CASTLE_NOEXCEPT { return iterator(&before_); }

    /**
     * @brief Returns a const iterator to the sentinel before the first element.
     * @return Const iterator suitable for algorithms that need the pre-head sentinel.
     */
    const_iterator before_begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(&before_); }

    /**
     * @brief Returns the first element.
     * @return Reference to the head value.
     * @warning The list must not be empty.
     */
    reference front() CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Returns the first element.
     * @return Const reference to the head value.
     * @warning The list must not be empty.
     */
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Inserts a copy of `value` at the front.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    status push_front(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return emplace_after(before_begin(), value);
    }

    /**
     * @brief Inserts `value` at the front by move construction.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    status push_front(T&& value) CASTLE_NOEXCEPT
    {
        return emplace_after(before_begin(), CASTLE_MOVE(value));
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
        return emplace_after(before_begin(), CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Removes the first element.
     * @return `status::ok` on success or `status::empty` when the list has no elements.
     * @note Complexity is O(1).
     */
    status pop_front() CASTLE_NOEXCEPT
    {
        if (head_ == nullptr)
        {
            return status::empty;
        }
        node* item = static_cast<node*>(head_);
        head_ = item->next;
        before_.next = head_;
        release_node(item);
        return status::ok;
    }

    /**
     * @brief Constructs an element after `position`.
     * @tparam Args Constructor argument types.
     * @param position Iterator naming the node before the insertion point.
     * @param args Arguments forwarded to `T`.
     * @return `status::ok` on success, `status::out_of_range` when `position == end()`, or
     * `status::full` when no free node remains.
     * @note Complexity is O(1).
     */
    template <typename... Args>
    status emplace_after(iterator position, Args&&... args) CASTLE_NOEXCEPT
    {
        if (position.node_ == nullptr)
        {
            return status::out_of_range;
        }
        node* item = allocate_node();
        if (item == nullptr)
        {
            return status::full;
        }
        memory::construct_at<T>(item->value_storage.address(0U), CASTLE_FORWARD<Args>(args)...);
        item->next = position.node_->next;
        position.node_->next = item;
        if (position.node_ == &before_)
        {
            head_ = item;
        }
        ++size_;
        return status::ok;
    }

    /**
     * @brief Erases the node immediately after `position`.
     * @param position Iterator naming the node before the element to erase.
     * @return Iterator to the element that followed the erased node, or `end()` when no element
     * was removed.
     * @note Complexity is O(1).
     */
    iterator erase_after(iterator position) CASTLE_NOEXCEPT
    {
        if (position.node_ == nullptr)
        {
            return end();
        }
        node_base* victim_base = position.node_->next;
        if (victim_base == nullptr)
        {
            return end();
        }
        node* victim = static_cast<node*>(victim_base);
        position.node_->next = victim->next;
        if (victim_base == head_)
        {
            head_ = victim->next;
        }
        iterator result(victim->next);
        release_node(victim);
        return result;
    }

    /**
     * @brief Removes every element equal to `value`.
     * @param value Value to remove.
     * @return Always `status::ok`.
     * @note Complexity is O(size()).
     */
    status remove(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        node_base* previous = &before_;
        node_base* current = before_.next;
        while (current != nullptr)
        {
            node* item = static_cast<node*>(current);
            if (*value_ptr(item) == value)
            {
                current = item->next;
                previous->next = current;
                if (item == static_cast<node*>(head_))
                {
                    head_ = current;
                }
                release_node(item);
                continue;
            }
            previous = current;
            current = current->next;
        }
        return status::ok;
    }

    /**
     * @brief Reverses the list in place.
     * @note Complexity is O(size()).
     */
    void reverse() CASTLE_NOEXCEPT
    {
        node_base* previous = nullptr;
        node_base* current = head_;
        while (current != nullptr)
        {
            node_base* next = current->next;
            current->next = previous;
            previous = current;
            current = next;
        }
        head_ = previous;
        before_.next = head_;
    }

    /**
     * @brief Destroys every element and returns all nodes to the free pool.
     * @note Complexity is O(size()).
     */
    void clear() CASTLE_NOEXCEPT
    {
        node_base* current = head_;
        while (current != nullptr)
        {
            node* item = static_cast<node*>(current);
            current = current->next;
            release_node(item);
        }
        head_ = nullptr;
        before_.next = nullptr;
    }

    /**
     * @brief Finds the first element equal to `value`.
     * @param value Value to search for.
     * @return Iterator to the first matching element, or `end()` when not found.
     * @note Complexity is O(size()).
     */
    iterator find(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        for (iterator it = begin(); it != end(); ++it)
        {
            if (*it == value)
            {
                return it;
            }
        }
        return end();
    }

    /**
     * @brief Finds the first element equal to `value`.
     * @param value Value to search for.
     * @return Const iterator to the first matching element, or `end()` when not found.
     * @note Complexity is O(size()).
     */
    const_iterator find(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (const_iterator it = begin(); it != end(); ++it)
        {
            if (*it == value)
            {
                return it;
            }
        }
        return end();
    }

private:
    /**
     * @brief Retrieves a pointer to the stored value from the given node.
     * @param item Node from which to retrieve the stored value.
     * @return Pointer to the stored value.
     */
    static T* value_ptr(node* item) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<T>(item->value_storage.address(0U));
    }

    /**
     * @brief Allocates a node from the free pool.
     * @return Pointer to the allocated node, or `nullptr` if the free pool is empty.
     */
    node* allocate_node() CASTLE_NOEXCEPT
    {
        if (free_head_ == nullptr)
        {
            return nullptr;
        }
        node* result = static_cast<node*>(free_head_);
        free_head_ = static_cast<node*>(result->next);
        result->next = nullptr;
        return result;
    }

    /**
     * @brief Releases a node back to the free pool.
     * @param item Node to be released.
     */
    void release_node(node* item) CASTLE_NOEXCEPT
    {
        memory::destroy_at(value_ptr(item));
        item->next = free_head_;
        free_head_ = item;
        if (size_ > 0U) // LCOV_EXCL_BR_LINE
        {
            --size_;
        }
    }

    /**
     * @brief Initializes the node pool for the forward list.
     * @note This function sets up the free list and resets the head and before nodes.
     */
    void initialise_pool() CASTLE_NOEXCEPT
    {
        free_head_ = nullptr;
        for (size_type i = N; i > 0U; --i)
        {
            nodes_[i - 1U].next = free_head_;
            free_head_ = &nodes_[i - 1U];
        }
        before_.next = nullptr;
        head_ = nullptr;
    }

    node nodes_[N];
    node_base* head_;
    size_type size_;
    node_base* free_head_;
    node_base before_;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_FORWARD_LIST_HPP
