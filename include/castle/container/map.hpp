#ifndef CASTLE_CONTAINER_MAP_HPP
#define CASTLE_CONTAINER_MAP_HPP

#include "castle/container/avl_tree.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/core/error_handler.hpp"

namespace castle
{
namespace container
{

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

    map() CASTLE_NOEXCEPT : tree_() {}
    explicit map(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT : tree_(compare) {}

    // Duplicate keys are ignored, matching std::map's initializer_list ctor.
    map(initializer_list<value_type> list) CASTLE_NOEXCEPT : tree_()
    {
        CASTLE_ASSERT(list.size() <= N, CASTLE_ERROR_GENERIC("map: initializer_list exceeds capacity")); // LCOV_EXCL_BR_LINE
        for (CASTLE_CONST value_type& entry : list)
        {
            // LCOV_EXCL_START
            if (full())
            {
                break;
            }
            // LCOV_EXCL_STOP
            insert(entry);
        }
    }

    ~map() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    map(CASTLE_CONST map&) CASTLE_DELETE;
    map& operator=(CASTLE_CONST map&) CASTLE_DELETE;
    map(map&&) CASTLE_DELETE;
    map& operator=(map&&) CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.size(); }
    size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.available(); }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.empty(); }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.full(); }

    iterator begin() CASTLE_NOEXCEPT { return tree_.begin(); }
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.begin(); }
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.cbegin(); }
    iterator end() CASTLE_NOEXCEPT { return tree_.end(); }
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.end(); }
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return tree_.cend(); }

    status insert(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        return tree_.insert(value.first, value.second);
    }

    status insert(CASTLE_CONST key_type& key, CASTLE_CONST mapped_type& value) CASTLE_NOEXCEPT
    {
        return tree_.insert(key, value);
    }

    status insert(CASTLE_CONST key_type& key, mapped_type&& value) CASTLE_NOEXCEPT
    {
        return tree_.insert(key, CASTLE_MOVE(value));
    }

    template <typename... Args>
    status try_emplace(CASTLE_CONST key_type& key, Args&&... args) CASTLE_NOEXCEPT
    {
        return tree_.emplace(key, CASTLE_FORWARD<Args>(args)...);
    }

    iterator find(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.find(key);
    }

    const_iterator find(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.find(key);
    }

    bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.contains(key);
    }

    mapped_type* get(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.mapped(key);
    }

    CASTLE_CONST mapped_type* get(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.mapped(key);
    }

    iterator lower_bound(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.lower_bound(key);
    }

    const_iterator lower_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.lower_bound(key);
    }

    iterator upper_bound(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.upper_bound(key);
    }

    const_iterator upper_bound(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_.upper_bound(key);
    }

    status erase(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        return tree_.erase(key);
    }

    iterator erase(iterator position) CASTLE_NOEXCEPT
    {
        return tree_.erase(position);
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

#endif // CASTLE_CONTAINER_MAP_HPP
