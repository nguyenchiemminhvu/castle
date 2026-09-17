#ifndef CASTLE_CONTAINER_SET_H
#define CASTLE_CONTAINER_SET_H

#include "castle/container/avl_tree.h"

namespace castle
{
namespace container
{
namespace detail
{
struct set_value
{
};
} // namespace detail

template <typename Key, size_type N, typename Compare = castle::less<Key>>
class set
{
    using tree_type = avl_tree<Key, detail::set_value, N, Compare>;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;

    class const_iterator
    {
    public:
        using iterator_category = bidirectional_iterator_tag;
        using value_type = Key;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST Key*;
        using reference = CASTLE_CONST Key&;

        const_iterator() CASTLE_NOEXCEPT : it_() {}
        explicit const_iterator(typename tree_type::const_iterator it) CASTLE_NOEXCEPT : it_(it) {}
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return it_->first; }
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return &it_->first; }
        const_iterator& operator++() CASTLE_NOEXCEPT { ++it_; return *this; }
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }
        const_iterator& operator--() CASTLE_NOEXCEPT { --it_; return *this; }
        const_iterator operator--(int) CASTLE_NOEXCEPT { const_iterator temp(*this); --(*this); return temp; }
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return it_ == other.it_; }
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }
    private:
        typename tree_type::const_iterator it_;
        friend class set;
    };

    using iterator = const_iterator;

    set() CASTLE_NOEXCEPT : tree_() {}
    explicit set(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT : tree_(compare) {}
    ~set() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    set(CASTLE_CONST set&) CASTLE_DELETE;
    set& operator=(CASTLE_CONST set&) CASTLE_DELETE;
    set(set&&) CASTLE_DELETE;
    set& operator=(set&&) CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.size(); }
    size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.available(); }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.empty(); }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.full(); }

    iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(tree_.cbegin()); }
    iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(tree_.cend()); }
    iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }
    iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    status insert(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        detail::set_value value;
        return tree_.insert(key, value);
    }

    iterator find(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(tree_.find(key));
    }

    bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.contains(key);
    }

    iterator lower_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(tree_.lower_bound(key));
    }

    iterator upper_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(tree_.upper_bound(key));
    }

    status erase(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.erase(key);
    }

    void clear() CASTLE_NOEXCEPT
    {
        tree_.clear();
    }

private:
    tree_type tree_;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_SET_H
