// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file avl_tree.hpp
 * @brief Fixed-capacity associative AVL tree with parent links and deterministic node storage.
 *
 * Use this container when ordered lookup is required without dynamic allocation and the maximum
 * node count is known at compile time. Nodes are stored in an internal pool, tree searches,
 * insertions, erasures, `lower_bound()`, and `upper_bound()` are O(log N) on the balanced tree,
 * and in-order iteration is supported through bidirectional iterators. AVL rebalancing performs
 * single or double rotations after insertions and erasures to keep subtree heights within one.
 *
 * @note Node addresses remain stable while the node is present in the tree because nodes are
 * never relocated, only relinked.
 * @warning Capacity is fixed at compile time; insertions return `status::full` when the node
 * pool is exhausted.
 */
#ifndef CASTLE_CONTAINER_AVL_TREE_HPP
#define CASTLE_CONTAINER_AVL_TREE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/iterator/tags.hpp"
#include "castle/error/status.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/utility/compare.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/pair.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity ordered key/value tree that maintains AVL balance.
 * @tparam Key Key type.
 * @tparam T Mapped value type.
 * @tparam N Maximum number of nodes.
 * @tparam Compare Strict weak ordering used on keys.
 * @note Iteration visits elements in ascending key order as defined by `Compare`.
 * @warning The tree is non-copyable and non-movable because it owns in-place node storage.
 */
template <typename Key, typename T, size_type N, typename Compare = castle::less<Key>>
class avl_tree
{
    static_assert(N > 0U, "avl_tree requires a positive capacity");
    static_assert(meta::is_destructible<Key>::value, "avl_tree key must be destructible");
    static_assert(meta::is_destructible<T>::value, "avl_tree mapped type must be destructible");

    using node_value_type = castle::pair<CASTLE_CONST Key, T>;

    struct node
    {
        node()
            : left(nullptr)
            , right(nullptr)
            , parent(nullptr)
            , height(1U)
            , used(false)
            , value_storage()
        {
        }

        node* left;
        node* right;
        node* parent;
        uint8_t height;
        bool used;
        memory::static_storage<node_value_type, 1U> value_storage;
    };

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = node_value_type;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = value_type&;
    using const_reference = CASTLE_CONST value_type&;
    using pointer = value_type*;
    using const_pointer = CASTLE_CONST value_type*;

    class const_iterator;

    /**
     * @brief Bidirectional iterator over mutable tree elements in sorted key order.
     * @note Increment moves to the in-order successor and decrement moves to the in-order
     * predecessor, each in O(log N) worst case and O(1) amortized over a full traversal.
     */
    class iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
        using value_type = avl_tree::value_type;
        using difference_type = castle::difference_type;
        using pointer = value_type*;
        using reference = value_type&;

        /**
         * @brief Constructs an end iterator.
         */
        iterator() CASTLE_NOEXCEPT
            : node_(nullptr) {}

        /**
         * @brief Constructs an iterator from an internal node.
         * @param value Node addressed by the iterator.
         */
        explicit iterator(node* value) CASTLE_NOEXCEPT
            : node_(value) {}

        /**
         * @brief Dereferences the current node.
         * @return Reference to the current key/value pair.
         * @warning The iterator must not equal `end()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return *tree_value(node_);
        }

        /**
         * @brief Returns the address of the current node value.
         * @return Pointer to the current key/value pair.
         * @warning The iterator must not equal `end()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return tree_value(node_);
        }

        /**
         * @brief Advances to the in-order successor.
         * @return Reference to `*this`.
         */
        iterator& operator++() CASTLE_NOEXCEPT
        {
            node_ = successor(node_);
            return *this;
        }

        /**
         * @brief Returns the current iterator and then advances to the successor.
         * @return Iterator state before incrementing.
         */
        iterator operator++(int) CASTLE_NOEXCEPT
        {
            iterator temp(*this);
            ++(*this);
            return temp;
        }

        /**
         * @brief Moves to the in-order predecessor.
         * @return Reference to `*this`.
         */
        iterator& operator--() CASTLE_NOEXCEPT
        {
            node_ = predecessor(node_);
            return *this;
        }

        /**
         * @brief Returns the current iterator and then moves to the predecessor.
         * @return Iterator state before decrementing.
         */
        iterator operator--(int) CASTLE_NOEXCEPT
        {
            iterator temp(*this);
            --(*this);
            return temp;
        }

        /**
         * @brief Tests whether two iterators reference the same node.
         * @param other Iterator to compare with.
         * @return `true` when both iterators reference the same node.
         */
        bool operator==(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return node_ == other.node_;
        }

        /**
         * @brief Tests whether two iterators reference different nodes.
         * @param other Iterator to compare with.
         * @return `true` when the iterators differ.
         */
        bool operator!=(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return !(*this == other);
        }

        /**
         * @brief Converts to a const iterator.
         * @return Const iterator referencing the same node.
         */
        operator const_iterator() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return const_iterator(node_);
        }

    private:
        friend class avl_tree;
        friend class const_iterator;

        /**
         * @brief Retrieves the value stored in a tree node.
         *
         * @param item Node whose value is to be retrieved.
         * @return Pointer to the value stored in the node.
         * @note The returned pointer may be `nullptr` if the node does not contain a value.
         */
        static value_type* tree_value(node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<value_type>(item->value_storage.address(0U));
        }

        /**
         * @brief Finds the in-order successor of a given tree node.
         *
         * @param item Node whose successor is to be found.
         * @return Pointer to the successor node, or `nullptr` if no successor exists.
         * @note The returned pointer may be `nullptr` if the node has no successor.
         */
        static node* successor(node* item) CASTLE_NOEXCEPT
        {
            // LCOV_EXCL_START
            if (item == nullptr)
            {
                return nullptr;
            }

            if (item->right != nullptr)
            {
                item = item->right;
                while (item->left != nullptr)
                {
                    item = item->left;
                }
                return item;
            }
            // LCOV_EXCL_STOP

            node* parent = item->parent;
            while ((parent != nullptr) && (item == parent->right))
            {
                item = parent;
                parent = parent->parent;
            }
            return parent;
        }

        /**
         * @brief Finds the in-order predecessor of a given tree node.
         *
         * @param item Node whose predecessor is to be found.
         * @return Pointer to the predecessor node, or `nullptr` if no predecessor exists.
         * @note The returned pointer may be `nullptr` if the node has no predecessor.
         */
        static node* predecessor(node* item) CASTLE_NOEXCEPT
        {
            // LCOV_EXCL_START
            if (item == nullptr)
            {
                return nullptr;
            }

            if (item->left != nullptr)
            {
                item = item->left;
                while (item->right != nullptr)
                {
                    item = item->right;
                }
                return item;
            }

            node* parent = item->parent;
            while ((parent != nullptr) && (item == parent->left))
            {
                item = parent;
                parent = parent->parent;
            }
            // LCOV_EXCL_STOP
            return parent;
        }
        node* node_;
    };

    /**
     * @brief Bidirectional iterator over constant tree elements in sorted key order.
     * @note Increment and decrement follow in-order successor/predecessor links.
     */
    class const_iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
        using value_type = avl_tree::value_type;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST value_type*;
        using reference = CASTLE_CONST value_type&;

        /**
         * @brief Constructs an end iterator.
         */
        const_iterator() CASTLE_NOEXCEPT : node_(nullptr) {}

        /**
         * @brief Constructs an iterator from an internal node.
         * @param value Node addressed by the iterator.
         */
        explicit const_iterator(CASTLE_CONST node* value) CASTLE_NOEXCEPT : node_(value) {}

        /**
         * @brief Constructs a const iterator from a mutable iterator.
         * @param other Mutable iterator to copy.
         */
        const_iterator(CASTLE_CONST iterator& other) CASTLE_NOEXCEPT : node_(other.node_) {}

        /**
         * @brief Dereferences the current node.
         * @return Const reference to the current key/value pair.
         * @warning The iterator must not equal `end()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *tree_value(node_); }

        /**
         * @brief Returns the address of the current node value.
         * @return Const pointer to the current key/value pair.
         * @warning The iterator must not equal `end()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return tree_value(node_); }

        /**
         * @brief Advances to the in-order successor.
         * @return Reference to `*this`.
         */
        const_iterator& operator++() CASTLE_NOEXCEPT { node_ = successor(node_); return *this; }

        /**
         * @brief Returns the current iterator and then advances to the successor.
         * @return Iterator state before incrementing.
         */
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }

        /**
         * @brief Moves to the in-order predecessor.
         * @return Reference to `*this`.
         */
        const_iterator& operator--() CASTLE_NOEXCEPT { node_ = predecessor(node_); return *this; }

        /**
         * @brief Returns the current iterator and then moves to the predecessor.
         * @return Iterator state before decrementing.
         */
        const_iterator operator--(int) CASTLE_NOEXCEPT { const_iterator temp(*this); --(*this); return temp; }

        /**
         * @brief Tests whether two iterators reference the same node.
         * @param other Iterator to compare with.
         * @return `true` when both iterators reference the same node.
         */
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return node_ == other.node_; }

        /**
         * @brief Tests whether two iterators reference different nodes.
         * @param other Iterator to compare with.
         * @return `true` when the iterators differ.
         */
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

    private:
        friend class avl_tree;

        /**
         * @brief Retrieves the value stored in a tree node.
         * @param item Node whose value is to be retrieved.
         * @return Pointer to the value stored in the node.
         */
        static pointer tree_value(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<CASTLE_CONST value_type>(item->value_storage.address(0U));
        }

        /**
         * @brief Finds the in-order successor of a given tree node.
         * @param item Node whose successor is to be found.
         * @return Pointer to the successor node, or `nullptr` if no successor exists.
         */
        static CASTLE_CONST node* successor(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return iterator::successor(const_cast<node*>(item));
        }

        /**
         * @brief Finds the in-order predecessor of a given tree node.
         * @param item Node whose predecessor is to be found.
         * @return Pointer to the predecessor node, or `nullptr` if no predecessor exists.
         */
        static CASTLE_CONST node* predecessor(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return iterator::predecessor(const_cast<node*>(item));
        }

        CASTLE_CONST node* node_;
    };

    /**
     * @brief Constructs an empty tree.
     * @note Complexity is O(N) because the node pool is initialized.
     */
    avl_tree() CASTLE_NOEXCEPT : root_(nullptr), free_head_(nullptr), size_(0U), compare_()
    {
        initialise_pool();
    }

    /**
     * @brief Constructs an empty tree with a caller-supplied comparator.
     * @param compare Key comparator.
     * @note Complexity is O(N) because the node pool is initialized.
     */
    explicit avl_tree(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT
        : root_(nullptr), free_head_(nullptr), size_(0U), compare_(compare)
    {
        initialise_pool();
    }

    /**
     * @brief Destroys all nodes in the tree.
     * @note Complexity is O(size()).
     */
    ~avl_tree() CASTLE_NOEXCEPT
    {
        clear();
    }

    avl_tree(CASTLE_CONST avl_tree&) CASTLE_DELETE;
    avl_tree& operator=(CASTLE_CONST avl_tree&) CASTLE_DELETE;
    avl_tree(avl_tree&&) CASTLE_DELETE;
    avl_tree& operator=(avl_tree&&) CASTLE_DELETE;

    /**
     * @brief Returns the fixed compile-time node capacity.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the number of stored nodes.
     * @return Current node count.
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the number of free nodes remaining in the pool.
     * @return `capacity() - size()`.
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    /**
     * @brief Tests whether the tree is empty.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Tests whether the node pool is exhausted.
     * @return `true` when `size() == capacity()`.
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Returns an iterator to the smallest key.
     * @return Iterator to the minimum node or `end()` when the tree is empty.
     */
    iterator begin() CASTLE_NOEXCEPT { return iterator(minimum(root_)); }

    /**
     * @brief Returns a const iterator to the smallest key.
     * @return Const iterator to the minimum node or `end()` when the tree is empty.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(minimum(root_)); }

    /**
     * @brief Returns a const iterator to the smallest key.
     * @return Const iterator equal to `begin()`.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns the past-the-end iterator.
     * @return Iterator whose node pointer is null.
     */
    iterator end() CASTLE_NOEXCEPT { return iterator(nullptr); }

    /**
     * @brief Returns the past-the-end const iterator.
     * @return Const iterator whose node pointer is null.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(nullptr); }

    /**
     * @brief Returns the past-the-end const iterator.
     * @return Const iterator equal to `end()`.
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /**
     * @brief Inserts a key/value pair by copy.
     * @param key Key to insert.
     * @param value Mapped value to associate with `key`.
     * @return `status::ok` on success, `status::already_exists` when `key` is present, or
     * `status::full` when no free node remains.
     * @note Complexity is O(log N). Successful insertion may trigger AVL single or double
     * rotations while walking back toward the root.
     */
    status insert(CASTLE_CONST Key& key, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return emplace_value(key, value);
    }

    /**
     * @brief Inserts a key/value pair by move.
     * @param key Key to insert.
     * @param value Mapped value to associate with `key`.
     * @return `status::ok` on success, `status::already_exists` when `key` is present, or
     * `status::full` when no free node remains.
     * @note Complexity is O(log N).
     */
    status insert(CASTLE_CONST Key& key, T&& value) CASTLE_NOEXCEPT
    {
        return emplace_value(key, CASTLE_MOVE(value));
    }

    /**
     * @brief Constructs a mapped value in place and inserts it under `key`.
     * @tparam Args Constructor argument types for `T`.
     * @param key Key to insert.
     * @param args Arguments forwarded to `T`.
     * @return `status::ok` on success, `status::already_exists` when `key` is present, or
     * `status::full` when no free node remains.
     * @note Complexity is O(log N). Successful insertion may rebalance the path to the root.
     */
    template <typename... Args>
    status emplace(CASTLE_CONST Key& key, Args&&... args) CASTLE_NOEXCEPT
    {
        if (find_node(key) != nullptr)
        {
            return status::already_exists;
        }

        T value(CASTLE_FORWARD<Args>(args)...);
        return emplace_value(key, CASTLE_MOVE(value));
    }

    /**
     * @brief Finds a node by key.
     * @param key Key to search for.
     * @return Iterator to the matching node or `end()` when absent.
     * @note Complexity is O(log N).
     */
    iterator find(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        return iterator(find_node(key));
    }

    /**
     * @brief Finds a node by key.
     * @param key Key to search for.
     * @return Const iterator to the matching node or `end()` when absent.
     * @note Complexity is O(log N).
     */
    const_iterator find(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(find_node(key));
    }

    /**
     * @brief Tests whether a key exists in the tree.
     * @param key Key to search for.
     * @return `true` when a matching key is present.
     * @note Complexity is O(log N).
     */
    bool contains(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_node(key) != nullptr;
    }

    /**
     * @brief Returns the first node whose key is not less than `key`.
     * @param key Lower-bound search key.
     * @return Iterator to the first node satisfying `!compare(node_key, key)`, or `end()` when no
     * such node exists.
     * @note Complexity is O(log N).
     */
    iterator lower_bound(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        node* current = root_;
        node* result = nullptr;
        while (current != nullptr)
        {
            CASTLE_CONST Key& current_key = value(current).first;
            if (compare_(current_key, key))
            {
                current = current->right;
            }
            else
            {
                result = current;
                current = current->left;
            }
        }
        return iterator(result);
    }

    /**
     * @brief Returns the first node whose key is not less than `key`.
     * @param key Lower-bound search key.
     * @return Const iterator to the lower-bound node or `end()` when absent.
     * @note Complexity is O(log N).
     */
    const_iterator lower_bound(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(const_cast<avl_tree*>(this)->lower_bound(key).node_);
    }

    /**
     * @brief Returns the first node whose key is greater than `key`.
     * @param key Upper-bound search key.
     * @return Iterator to the first node satisfying `compare(key, node_key)`, or `end()` when no
     * such node exists.
     * @note Complexity is O(log N).
     */
    iterator upper_bound(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        node* current = root_;
        node* result = nullptr;
        while (current != nullptr)
        {
            CASTLE_CONST Key& current_key = value(current).first;
            if (compare_(key, current_key))
            {
                result = current;
                current = current->left;
            }
            else
            {
                current = current->right;
            }
        }
        return iterator(result);
    }

    /**
     * @brief Returns the first node whose key is greater than `key`.
     * @param key Upper-bound search key.
     * @return Const iterator to the upper-bound node or `end()` when absent.
     * @note Complexity is O(log N).
     */
    const_iterator upper_bound(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(const_cast<avl_tree*>(this)->upper_bound(key).node_);
    }

    /**
     * @brief Erases a node by key.
     * @param key Key to erase.
     * @return `status::ok` on success or `status::not_found` when the key is absent.
     * @note Complexity is O(log N). Erasure may trigger AVL single or double rotations while
     * walking back toward the root.
     */
    status erase(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        node* item = find_node(key);
        if (item == nullptr)
        {
            return status::not_found;
        }
        erase_node(item);
        return status::ok;
    }

    /**
     * @brief Erases the node referenced by `position`.
     * @param position Iterator naming the node to erase.
     * @return Iterator to the in-order successor of the erased node, or `end()` when `position`
     * was already `end()`.
     * @note Complexity is O(log N).
     */
    iterator erase(iterator position) CASTLE_NOEXCEPT
    {
        if (position.node_ == nullptr)
        {
            return end();
        }
        node* next = iterator::successor(position.node_);
        erase_node(position.node_);
        return iterator(next);
    }

    /**
     * @brief Removes every node from the tree and resets the node pool.
     * @note Complexity is O(size() + N).
     */
    void clear() CASTLE_NOEXCEPT
    {
        for (node* current = minimum(root_); current != nullptr; )
        {
            node* next = iterator::successor(current);
            release_node(current);
            current = next;
        }
        root_ = nullptr;
        size_ = 0U;
        initialise_pool();
    }

    /**
     * @brief Returns a pointer to the mapped value for `key`.
     * @param key Key to search for.
     * @return Pointer to the mapped value or `nullptr` when the key is absent.
     * @note Complexity is O(log N).
     */
    mapped_type* mapped(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        node* item = find_node(key);
        return item == nullptr
               ? nullptr
               : &value(item).second;
    }

    /**
     * @brief Returns a pointer to the mapped value for `key`.
     * @param key Key to search for.
     * @return Const pointer to the mapped value or `nullptr` when the key is absent.
     * @note Complexity is O(log N).
     */
    CASTLE_CONST mapped_type* mapped(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST node* item = find_node(key);
        return item == nullptr
               ? nullptr
               : &value(item).second;
    }

private:
    /**
     * @brief Inserts a new key-value pair into the tree if the key does not already exist.
     * @param key Key to insert.
     * @param mapped_value Mapped value to associate with the key.
     * @return Status indicating the result of the insertion attempt.
     * @note Complexity is O(log N).
     */
    template <typename ValueArg>
    status emplace_value(CASTLE_CONST Key& key, ValueArg&& mapped_value) CASTLE_NOEXCEPT
    {
        node* current = root_;
        node* parent = nullptr;
        bool insert_left = false;
        while (current != nullptr)
        {
            parent = current;
            CASTLE_CONST Key& current_key = value(current).first;
            if (compare_(key, current_key))
            {
                current = current->left;
                insert_left = true;
            }
            else if (compare_(current_key, key))
            {
                current = current->right;
                insert_left = false;
            }
            else
            {
                return status::already_exists;
            }
        }

        node* item = allocate_node();
        if (item == nullptr)
        {
            return status::full;
        }
        memory::construct_at<value_type>(
            item->value_storage.address(0U),
            key,
            CASTLE_FORWARD<ValueArg>(mapped_value)
        );
        item->parent = parent;
        if (parent == nullptr)
        {
            root_ = item;
        }
        else if (insert_left)
        {
            parent->left = item;
        }
        else
        {
            parent->right = item;
        }
        ++size_;
        rebalance(parent);
        return status::ok;
    }

    /**
     * @brief Retrieves the value stored in a given tree node.
     * @param item Node whose value is to be retrieved.
     * @return Reference to the value stored in the node.
     */
    static value_type& value(node* item) CASTLE_NOEXCEPT
    {
        return *memory::object_from_address<value_type>(item->value_storage.address(0U));
    }

    /**
     * @brief Retrieves the value stored in a given tree node (const version).
     * @param item Node whose value is to be retrieved.
     * @return Const reference to the value stored in the node.
     */
    static CASTLE_CONST value_type& value(CASTLE_CONST node* item) CASTLE_NOEXCEPT
    {
        return *memory::object_from_address<CASTLE_CONST value_type>(item->value_storage.address(0U));
    }

    /**
     * @brief Allocates a new node from the free list.
     * @return Pointer to the newly allocated node, or `nullptr` if the free list is empty.
     */
    node* allocate_node() CASTLE_NOEXCEPT
    {
        if (free_head_ == nullptr)
        {
            return nullptr;
        }
        node* result = free_head_;
        free_head_ = result->left;
        result->left = nullptr;
        result->right = nullptr;
        result->parent = nullptr;
        result->height = 1U;
        result->used = true;
        return result;
    }

    /**
     * @brief Releases a node back to the free list.
     * @param item Node to be released.
     */
    void release_node(node* item) CASTLE_NOEXCEPT
    {
        memory::destroy_at(value_ptr(item));
        item->used = false;
        item->left = free_head_;
        item->right = nullptr;
        item->parent = nullptr;
        item->height = 0U;
        free_head_ = item;
        if (size_ > 0U)
        {
            --size_;
        }
    }

    /**
     * @brief Retrieves a pointer to the value stored in a given tree node.
     * @param item Node whose value pointer is to be retrieved.
     * @return Pointer to the value stored in the node.
     */
    value_type* value_ptr(node* item) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<value_type>(item->value_storage.address(0U));
    }

    /**
     * @brief Finds a node with the specified key in the tree.
     * @param key Key to search for in the tree.
     * @return Pointer to the node with the specified key, or `nullptr` if no such node exists.
     */
    node* find_node(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        node* current = root_;
        while (current != nullptr)
        {
            CASTLE_CONST Key& current_key = value(current).first;
            if (compare_(key, current_key))
            {
                current = current->left;
            }
            else if (compare_(current_key, key))
            {
                current = current->right;
            }
            else
            {
                return current;
            }
        }
        return nullptr;
    }

    /**
     * @brief Finds the node with the minimum key in the subtree rooted at the given node.
     * @param item Root of the subtree to search for the minimum key.
     * @return Pointer to the node with the minimum key, or `nullptr` if the subtree is empty.
     */
    static node* minimum(node* item) CASTLE_NOEXCEPT
    {
        if (item == nullptr)
        {
            return nullptr;
        }
        while (item->left != nullptr)
        {
            item = item->left;
        }
        return item;
    }

    /**
     * @brief Retrieves the height of a given node.
     * @param item Node whose height is to be retrieved.
     * @return Height of the node, or 0 if the node is `nullptr`.
     */
    static uint8_t height(node* item) CASTLE_NOEXCEPT
    {
        return item == nullptr
               ? 0U
               : item->height;
    }

    /**
     * @brief Calculates the balance factor of a given node.
     * @param item Node whose balance factor is to be calculated.
     * @return Balance factor of the node.
     */
    static int balance_factor(node* item) CASTLE_NOEXCEPT
    {
        return static_cast<int>(height(item->left)) - static_cast<int>(height(item->right));
    }

    /**
     * @brief Updates the height of a given node based on the heights of its children.
     * @param item Node whose height is to be updated.
     */
    static void update_height(node* item) CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t left_height = height(item->left);
        CASTLE_CONST uint8_t right_height = height(item->right);
        item->height = static_cast<uint8_t>((left_height > right_height ? left_height : right_height) + 1U);
    }

    /**
     * @brief Replaces one subtree as a child of its parent with another subtree.
     * @param old_node The node to be replaced.
     * @param new_node The node to replace `old_node` with.
     * @note This function does not update the heights of the affected nodes.
     */
    void transplant(node* old_node, node* new_node) CASTLE_NOEXCEPT
    {
        if (old_node->parent == nullptr)
        {
            root_ = new_node;
        }
        else if (old_node == old_node->parent->left)
        {
            old_node->parent->left = new_node;
        }
        else
        {
            old_node->parent->right = new_node;
        }
        if (new_node != nullptr)
        {
            new_node->parent = old_node->parent;
        }
    }

    /**
     * @brief Performs a left rotation around the given pivot node.
     * @param pivot Node around which the left rotation is to be performed.
     */
    void rotate_left(node* pivot) CASTLE_NOEXCEPT
    {
        node* child = pivot->right;
        pivot->right = child->left;
        if (child->left != nullptr)
        {
            child->left->parent = pivot;
        }
        child->parent = pivot->parent;
        if (pivot->parent == nullptr)
        {
            root_ = child;
        }
        else if (pivot == pivot->parent->left)
        {
            pivot->parent->left = child;
        }
        else
        {
            pivot->parent->right = child;
        }
        child->left = pivot;
        pivot->parent = child;
        update_height(pivot);
        update_height(child);
    }

    /**
     * @brief Performs a right rotation around the given pivot node.
     * @param pivot Node around which the right rotation is to be performed.
     */
    void rotate_right(node* pivot) CASTLE_NOEXCEPT
    {
        node* child = pivot->left;
        pivot->left = child->right;
        if (child->right != nullptr)
        {
            child->right->parent = pivot;
        }
        child->parent = pivot->parent;
        if (pivot->parent == nullptr)
        {
            root_ = child;
        }
        else if (pivot == pivot->parent->left)
        {
            pivot->parent->left = child;
        }
        else
        {
            pivot->parent->right = child;
        }
        child->right = pivot;
        pivot->parent = child;
        update_height(pivot);
        update_height(child);
    }

    /**
     * @brief Rebalances the AVL tree starting from the given node and moving up to the root.
     * @param current Node from which to start rebalancing.
     */
    void rebalance(node* current) CASTLE_NOEXCEPT
    {
        while (current != nullptr)
        {
            update_height(current);
            CASTLE_CONST int balance = balance_factor(current);
            if (balance > 1)
            {
                // Left-heavy subtree: pre-rotate the child on LR cases, then rotate the pivot.
                if (balance_factor(current->left) < 0) rotate_left(current->left);
                rotate_right(current);
            }
            else if (balance < -1)
            {
                // Right-heavy subtree: pre-rotate the child on RL cases, then rotate the pivot.
                if (balance_factor(current->right) > 0) rotate_right(current->right);
                rotate_left(current);
            }
            current = current->parent;
        }
    }

    /**
     * @brief Erases the specified node from the AVL tree and rebalances the tree if necessary.
     * @param item Node to be erased from the AVL tree.
     */
    void erase_node(node* item) CASTLE_NOEXCEPT
    {
        node* rebalance_from = item->parent;
        if ((item->left == nullptr) || (item->right == nullptr))
        {
            node* child = item->left != nullptr
                          ? item->left
                          : item->right;
            transplant(item, child);
            release_node(item);
            rebalance(rebalance_from != nullptr // LCOV_EXCL_BR_LINE
                      ? rebalance_from
                      : child);
            return;
        }

        node* successor_node = minimum(item->right);
        node* successor_parent = successor_node->parent;
        if (successor_parent != item)
        {
            transplant(successor_node, successor_node->right);
            successor_node->right = item->right;
            successor_node->right->parent = successor_node;
            rebalance_from = successor_parent;
        }
        else
        {
            rebalance_from = successor_node;
        }

        transplant(item, successor_node);
        successor_node->left = item->left;
        successor_node->left->parent = successor_node;
        update_height(successor_node);
        release_node(item);
        rebalance(rebalance_from);
    }

    /**
     * @brief Initializes the node pool for the AVL tree.
     */
    void initialise_pool() CASTLE_NOEXCEPT
    {
        free_head_ = nullptr;
        for (size_type i = N; i > 0U; --i)
        {
            node& item = nodes_[i - 1U];
            item.left = free_head_;
            item.right = nullptr;
            item.parent = nullptr;
            item.height = 0U;
            item.used = false;
            free_head_ = &item;
        }
    }

    node nodes_[N];
    node* root_;
    node* free_head_;
    size_type size_;
    Compare compare_;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_AVL_TREE_HPP
