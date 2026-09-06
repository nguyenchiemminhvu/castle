#ifndef CASTLE_CONTAINER_AVL_TREE_H
#define CASTLE_CONTAINER_AVL_TREE_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"
#include "castle/iterator/tags.h"
#include "castle/error/status.h"
#include "castle/memory/object.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/static_storage.h"
#include "castle/utility/compare.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"
#include "castle/utility/pair.h"

namespace castle
{
namespace container
{

// Fixed-capacity AVL tree with parent links. Nodes never move, erase does not
// reallocate, and all rebalancing work is bounded by O(log N).
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

    class iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
        using value_type = avl_tree::value_type;
        using difference_type = castle::difference_type;
        using pointer = value_type*;
        using reference = value_type&;

        iterator() CASTLE_NOEXCEPT
            : node_(nullptr) {}
        explicit iterator(node* value) CASTLE_NOEXCEPT
            : node_(value) {}

        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return *tree_value(node_);
        }

        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return tree_value(node_);
        }

        iterator& operator++() CASTLE_NOEXCEPT
        {
            node_ = successor(node_);
            return *this;
        }

        iterator operator++(int) CASTLE_NOEXCEPT
        {
            iterator temp(*this);
            ++(*this);
            return temp;
        }

        iterator& operator--() CASTLE_NOEXCEPT
        {
            node_ = predecessor(node_);
            return *this;
        }

        iterator operator--(int) CASTLE_NOEXCEPT
        {
            iterator temp(*this);
            --(*this);
            return temp;
        }

        bool operator==(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return node_ == other.node_;
        }

        bool operator!=(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return !(*this == other);
        }

        operator const_iterator() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return const_iterator(node_);
        }

    private:
        friend class avl_tree;
        friend class const_iterator;

        static value_type* tree_value(node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<value_type>(item->value_storage.address(0U));
        }

        static node* successor(node* item) CASTLE_NOEXCEPT
        {
            if (item == nullptr) return nullptr;
            if (item->right != nullptr)
            {
                item = item->right;
                while (item->left != nullptr)
                {
                    item = item->left;
                }
                return item;
            }
            node* parent = item->parent;
            while ((parent != nullptr) && (item == parent->right))
            {
                item = parent;
                parent = parent->parent;
            }
            return parent;
        }

        static node* predecessor(node* item) CASTLE_NOEXCEPT
        {
            if (item == nullptr) return nullptr;
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
            return parent;
        }
        node* node_;
    };

    class const_iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
        using value_type = avl_tree::value_type;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST value_type*;
        using reference = CASTLE_CONST value_type&;

        const_iterator() CASTLE_NOEXCEPT : node_(nullptr) {}
        explicit const_iterator(CASTLE_CONST node* value) CASTLE_NOEXCEPT : node_(value) {}
        const_iterator(CASTLE_CONST iterator& other) CASTLE_NOEXCEPT : node_(other.node_) {}

        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *tree_value(node_); }
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return tree_value(node_); }
        const_iterator& operator++() CASTLE_NOEXCEPT { node_ = successor(node_); return *this; }
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }
        const_iterator& operator--() CASTLE_NOEXCEPT { node_ = predecessor(node_); return *this; }
        const_iterator operator--(int) CASTLE_NOEXCEPT { const_iterator temp(*this); --(*this); return temp; }
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return node_ == other.node_; }
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

    private:
        friend class avl_tree;
        static pointer tree_value(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<CASTLE_CONST value_type>(item->value_storage.address(0U));
        }
        static CASTLE_CONST node* successor(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return iterator::successor(const_cast<node*>(item));
        }
        static CASTLE_CONST node* predecessor(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return iterator::predecessor(const_cast<node*>(item));
        }
        CASTLE_CONST node* node_;
    };

    avl_tree() CASTLE_NOEXCEPT : root_(nullptr), free_head_(nullptr), size_(0U), compare_()
    {
        initialise_pool();
    }

    explicit avl_tree(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT
        : root_(nullptr), free_head_(nullptr), size_(0U), compare_(compare)
    {
        initialise_pool();
    }

    ~avl_tree() CASTLE_NOEXCEPT
    {
        clear();
    }

    avl_tree(CASTLE_CONST avl_tree&) CASTLE_DELETE;
    avl_tree& operator=(CASTLE_CONST avl_tree&) CASTLE_DELETE;
    avl_tree(avl_tree&&) CASTLE_DELETE;
    avl_tree& operator=(avl_tree&&) CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    iterator begin() CASTLE_NOEXCEPT { return iterator(minimum(root_)); }
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(minimum(root_)); }
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }
    iterator end() CASTLE_NOEXCEPT { return iterator(nullptr); }
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(nullptr); }
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    status insert(CASTLE_CONST Key& key, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return emplace_value(key, value);
    }

    status insert(CASTLE_CONST Key& key, T&& value) CASTLE_NOEXCEPT
    {
        return emplace_value(key, CASTLE_MOVE(value));
    }

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

    iterator find(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        return iterator(find_node(key));
    }

    const_iterator find(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(find_node(key));
    }

    bool contains(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_node(key) != nullptr;
    }

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

    const_iterator lower_bound(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(const_cast<avl_tree*>(this)->lower_bound(key).node_);
    }

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

    const_iterator upper_bound(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(const_cast<avl_tree*>(this)->upper_bound(key).node_);
    }

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

    mapped_type* mapped(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        node* item = find_node(key);
        return item == nullptr
               ? nullptr
               : &value(item).second;
    }

    CASTLE_CONST mapped_type* mapped(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST node* item = find_node(key);
        return item == nullptr
               ? nullptr
               : &value(item).second;
    }

private:
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

    static value_type& value(node* item) CASTLE_NOEXCEPT
    {
        return *memory::object_from_address<value_type>(item->value_storage.address(0U));
    }

    static CASTLE_CONST value_type& value(CASTLE_CONST node* item) CASTLE_NOEXCEPT
    {
        return *memory::object_from_address<CASTLE_CONST value_type>(item->value_storage.address(0U));
    }

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

    value_type* value_ptr(node* item) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<value_type>(item->value_storage.address(0U));
    }

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

    static uint8_t height(node* item) CASTLE_NOEXCEPT
    {
        return item == nullptr
               ? 0U
               : item->height;
    }

    static int balance_factor(node* item) CASTLE_NOEXCEPT
    {
        return static_cast<int>(height(item->left)) - static_cast<int>(height(item->right));
    }

    static void update_height(node* item) CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t left_height = height(item->left);
        CASTLE_CONST uint8_t right_height = height(item->right);
        item->height = static_cast<uint8_t>((left_height > right_height ? left_height : right_height) + 1U);
    }

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

    void rebalance(node* current) CASTLE_NOEXCEPT
    {
        while (current != nullptr)
        {
            update_height(current);
            CASTLE_CONST int balance = balance_factor(current);
            if (balance > 1)
            {
                if (balance_factor(current->left) < 0) rotate_left(current->left);
                rotate_right(current);
            }
            else if (balance < -1)
            {
                if (balance_factor(current->right) > 0) rotate_right(current->right);
                rotate_left(current);
            }
            current = current->parent;
        }
    }

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
            rebalance(rebalance_from != nullptr
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

#endif // CASTLE_CONTAINER_AVL_TREE_H
