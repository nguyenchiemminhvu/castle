#ifndef CASTLE_CONTAINER_HASH_SET_H
#define CASTLE_CONTAINER_HASH_SET_H

#include "castle/container/hash_table.h"

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

    class const_iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = Key;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST Key*;
        using reference = CASTLE_CONST Key&;

        const_iterator() CASTLE_NOEXCEPT : it_() {}
        explicit const_iterator(typename table_type::const_iterator it) CASTLE_NOEXCEPT : it_(it) {}
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return it_->first; }
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return &it_->first; }
        const_iterator& operator++() CASTLE_NOEXCEPT { ++it_; return *this; }
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return it_ == other.it_; }
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }
    private:
        typename table_type::const_iterator it_;
        friend class hash_set;
    };

    using iterator = const_iterator;

    hash_set() CASTLE_NOEXCEPT : table_() {}
    explicit hash_set(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT : table_(hash) {}

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return table_.size(); }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return table_.available(); }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return table_.empty(); }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return table_.full(); }

    iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(table_.cbegin()); }
    iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return iterator(table_.cend()); }
    iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }
    iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    status insert(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return table_.insert(key, detail::hash_set_value());
    }

    bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return table_.contains(key);
    }

    iterator find(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return iterator(table_.find(key));
    }

    status erase(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return table_.erase(key);
    }

    void clear() CASTLE_NOEXCEPT
    {
        table_.clear();
    }

private:
    table_type table_;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_HASH_SET_H
