#ifndef CASTLE_CONTAINER_FORWARD_LIST_H
#define CASTLE_CONTAINER_FORWARD_LIST_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"
#include "castle/error/status.h"
#include "castle/memory/object.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/static_storage.h"
#include "castle/iterator/tags.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"

namespace castle
{
namespace container
{

// Fixed-capacity singly linked list. Nodes are allocated from an in-object
// pool and never move while linked into the list.
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

    class iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = T;
        using difference_type = castle::difference_type;
        using pointer = T*;
        using reference = T&;

        iterator() CASTLE_NOEXCEPT : node_(nullptr) {}
        explicit iterator(node_base* node_value) CASTLE_NOEXCEPT : node_(node_value) {}

        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *value_ptr(static_cast<node*>(node_)); }
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return value_ptr(static_cast<node*>(node_)); }
        iterator& operator++() CASTLE_NOEXCEPT { if (node_ != nullptr) node_ = node_->next; return *this; }
        iterator operator++(int) CASTLE_NOEXCEPT { iterator temp(*this); ++(*this); return temp; }
        bool operator==(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return node_ == other.node_; }
        bool operator!=(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }
        operator const_iterator() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(node_); }

    private:
        friend class forward_list;
        friend class const_iterator;
        static T* value_ptr(node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<T>(item->value_storage.address(0U));
        }
        node_base* node_;
    };

    class const_iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = T;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST T*;
        using reference = CASTLE_CONST T&;

        const_iterator() CASTLE_NOEXCEPT : node_(nullptr) {}
        explicit const_iterator(CASTLE_CONST node_base* node_value) CASTLE_NOEXCEPT : node_(node_value) {}
        const_iterator(CASTLE_CONST iterator& other) CASTLE_NOEXCEPT : node_(other.node_) {}
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *value_ptr(static_cast<CASTLE_CONST node*>(node_)); }
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return value_ptr(static_cast<CASTLE_CONST node*>(node_)); }
        const_iterator& operator++() CASTLE_NOEXCEPT { if (node_ != nullptr) node_ = node_->next; return *this; }
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return node_ == other.node_; }
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

    private:
        friend class forward_list;
        static pointer value_ptr(CASTLE_CONST node* item) CASTLE_NOEXCEPT
        {
            return memory::object_from_address<CASTLE_CONST T>(item->value_storage.address(0U));
        }
        CASTLE_CONST node_base* node_;
    };

    forward_list() CASTLE_NOEXCEPT : head_(nullptr), size_(0U), free_head_(nullptr), before_()
    {
        initialise_pool();
    }

    ~forward_list() CASTLE_NOEXCEPT { clear(); }

    forward_list(CASTLE_CONST forward_list&) CASTLE_DELETE;
    forward_list& operator=(CASTLE_CONST forward_list&) CASTLE_DELETE;
    forward_list(forward_list&&) CASTLE_DELETE;
    forward_list& operator=(forward_list&&) CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    iterator begin() CASTLE_NOEXCEPT { return iterator(head_); }
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(head_); }
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }
    iterator end() CASTLE_NOEXCEPT { return iterator(nullptr); }
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(nullptr); }
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    iterator before_begin() CASTLE_NOEXCEPT { return iterator(&before_); }
    const_iterator before_begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(&before_); }

    reference front() CASTLE_NOEXCEPT { return *begin(); }
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return *begin(); }

    status push_front(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return emplace_after(before_begin(), value);
    }

    status push_front(T&& value) CASTLE_NOEXCEPT
    {
        return emplace_after(before_begin(), CASTLE_MOVE(value));
    }

    template <typename... Args>
    status emplace_front(Args&&... args) CASTLE_NOEXCEPT
    {
        return emplace_after(before_begin(), CASTLE_FORWARD<Args>(args)...);
    }

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

    iterator find(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        for (iterator it = begin(); it != end(); ++it)
        {
            if (*it == value) return it;
        }
        return end();
    }

    const_iterator find(CASTLE_CONST T& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (const_iterator it = begin(); it != end(); ++it)
        {
            if (*it == value) return it;
        }
        return end();
    }

private:
    static T* value_ptr(node* item) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<T>(item->value_storage.address(0U));
    }

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

    void release_node(node* item) CASTLE_NOEXCEPT
    {
        memory::destroy_at(value_ptr(item));
        item->next = free_head_;
        free_head_ = item;
        if (size_ > 0U) --size_;
    }

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

#endif // CASTLE_CONTAINER_FORWARD_LIST_H
