#ifndef CASTLE_CONTAINER_HASH_TABLE_H
#define CASTLE_CONTAINER_HASH_TABLE_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/iterator/tags.h"
#include "castle/error/status.h"
#include "castle/memory/object.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/static_storage.h"
#include "castle/utility/compare.h"
#include "castle/utility/hash.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"
#include "castle/utility/pair.h"
#include <stdint.h>

namespace castle
{
namespace container
{

// Deterministic fixed-capacity open-addressed hash table. Probing is linear and
// bounded by exactly N slots; erase leaves tombstones until clear().
template <typename Key,
          typename T,
          size_type N,
          typename Hash = castle::hash<Key>,
          typename KeyEqual = castle::equal_to<Key>>
class hash_table
{
    static_assert(N > 0U, "hash_table requires a positive capacity");
    using entry_type = castle::pair<CASTLE_CONST Key, T>;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = entry_type;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = value_type&;
    using const_reference = CASTLE_CONST value_type&;

    class const_iterator;

    class iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = hash_table::value_type;
        using difference_type = castle::difference_type;
        using pointer = value_type*;
        using reference = value_type&;

        iterator() CASTLE_NOEXCEPT : table_(nullptr), index_(0U) {}
        iterator(hash_table* table, size_type index) CASTLE_NOEXCEPT : table_(table), index_(index) {}

        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *table_->entry_ptr(index_); }
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return table_->entry_ptr(index_); }

        iterator& operator++() CASTLE_NOEXCEPT
        {
            if (table_ != nullptr) table_->advance_index(index_);
            return *this;
        }

        iterator operator++(int) CASTLE_NOEXCEPT { iterator temp(*this); ++(*this); return temp; }
        bool operator==(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return table_ == other.table_ && index_ == other.index_; }
        bool operator!=(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

        operator const_iterator() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(table_, index_); }

    private:
        friend class hash_table;
        friend class const_iterator;
        hash_table* table_;
        size_type index_;
    };

    class const_iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = hash_table::value_type;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST value_type*;
        using reference = CASTLE_CONST value_type&;

        const_iterator() CASTLE_NOEXCEPT : table_(nullptr), index_(0U) {}
        const_iterator(CASTLE_CONST hash_table* table, size_type index) CASTLE_NOEXCEPT : table_(table), index_(index) {}
        const_iterator(CASTLE_CONST iterator& other) CASTLE_NOEXCEPT : table_(other.table_), index_(other.index_) {}

        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *table_->entry_ptr(index_); }
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return table_->entry_ptr(index_); }
        const_iterator& operator++() CASTLE_NOEXCEPT
        {
            if (table_ != nullptr) table_->advance_index(index_);
            return *this;
        }
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return table_ == other.table_ && index_ == other.index_; }
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

    private:
        friend class hash_table;
        CASTLE_CONST hash_table* table_;
        size_type index_;
    };

    hash_table() CASTLE_NOEXCEPT : size_(0U), hash_(), equal_(), states_{}
    {
    }

    explicit hash_table(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT
        : size_(0U), hash_(hash), equal_(), states_{}
    {
    }

    hash_table(CASTLE_CONST hash_table&) CASTLE_DELETE;
    hash_table& operator=(CASTLE_CONST hash_table&) CASTLE_DELETE;
    hash_table(hash_table&&) CASTLE_DELETE;
    hash_table& operator=(hash_table&&) CASTLE_DELETE;

    ~hash_table() CASTLE_NOEXCEPT { clear(); }

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    iterator begin() CASTLE_NOEXCEPT { return iterator(this, first_occupied()); }
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, first_occupied()); }
    iterator end() CASTLE_NOEXCEPT { return iterator(this, N); }
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, N); }
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    status insert(CASTLE_CONST Key& key, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return insert_value(key, value);
    }

    status insert(CASTLE_CONST Key& key, T&& value) CASTLE_NOEXCEPT
    {
        return insert_value(key, CASTLE_MOVE(value));
    }

    template <typename... Args>
    status emplace(CASTLE_CONST Key& key, Args&&... args) CASTLE_NOEXCEPT
    {
        if (find_index(key) != N)
        {
            return status::already_exists;
        }
        T value(CASTLE_FORWARD<Args>(args)...);
        return insert_value(key, CASTLE_MOVE(value));
    }

    // Constructs the mapped value only when the key is absent.
    template <typename... Args>
    status try_emplace(CASTLE_CONST Key& key, Args&&... args) CASTLE_NOEXCEPT
    {
        return emplace(key, CASTLE_FORWARD<Args>(args)...);
    }

    status insert_or_assign(CASTLE_CONST Key& key, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type existing = find_index(key);
        if (existing != N)
        {
            entry_ptr(existing)->second = value;
            return status::ok;
        }
        return insert(key, value);
    }

    status insert_or_assign(CASTLE_CONST Key& key, T&& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type existing = find_index(key);
        if (existing != N)
        {
            entry_ptr(existing)->second = CASTLE_MOVE(value);
            return status::ok;
        }
        return insert(key, CASTLE_MOVE(value));
    }

    iterator find(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        return iterator(this, find_index(key));
    }

    const_iterator find(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(this, find_index(key));
    }

    bool contains(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_index(key) != N;
    }

    mapped_type* get(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(key);
        return index == N
               ? nullptr
               : &entry_ptr(index)->second;
    }

    CASTLE_CONST mapped_type* get(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(key);
        return index == N
               ? nullptr
               : &entry_ptr(index)->second;
    }

    status erase(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(key);
        if (index == N)
        {
            return status::not_found;
        }
        memory::destroy_at(entry_ptr(index));
        states_[index] = deleted_state;
        --size_;
        return status::ok;
    }

    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            if (states_[i] == occupied_state)
            {
                memory::destroy_at(entry_ptr(i));
            }
            states_[i] = empty_state;
        }
        size_ = 0U;
    }

private:
    static CASTLE_CONSTEXPR uint8_t empty_state = 0U;
    static CASTLE_CONSTEXPR uint8_t occupied_state = 1U;
    static CASTLE_CONSTEXPR uint8_t deleted_state = 2U;

    template <typename ValueArg>
    status insert_value(CASTLE_CONST Key& key, ValueArg&& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type start = bucket(key);
        size_type first_deleted = N;
        for (size_type step = 0U; step < N; ++step)
        {
            CASTLE_CONST size_type index = (start + step) % N;
            if (states_[index] == occupied_state)
            {
                if (equal_(entry_ptr(index)->first, key))
                {
                    return status::already_exists;
                }
                continue;
            }
            if (states_[index] == deleted_state)
            {
                if (first_deleted == N)
                {
                    first_deleted = index;
                }
                continue;
            }
            CASTLE_CONST size_type target = first_deleted == N
                                            ? index
                                            : first_deleted;
            memory::construct_at<value_type>(slots_[target].address(0U), key, CASTLE_FORWARD<ValueArg>(value));
            states_[target] = occupied_state;
            ++size_;
            return status::ok;
        }
        if (first_deleted != N)
        {
            memory::construct_at<value_type>(
                slots_[first_deleted].address(0U),
                key,
                CASTLE_FORWARD<ValueArg>(value)
            );
            states_[first_deleted] = occupied_state;
            ++size_;
            return status::ok;
        }
        return status::full;
    }

    size_type bucket(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash_(key) % N;
    }

    size_type find_index(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type start = bucket(key);
        for (size_type step = 0U; step < N; ++step)
        {
            CASTLE_CONST size_type index = (start + step) % N;
            if (states_[index] == empty_state)
            {
                return N;
            }
            if ((states_[index] == occupied_state) && equal_(entry_ptr(index)->first, key))
            {
                return index;
            }
        }
        return N;
    }

    size_type first_occupied() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            if (states_[i] == occupied_state)
            {
                return i;
            }
        }
        return N;
    }

    void advance_index(size_type& index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= N)
        {
            return;
        }
        ++index;
        while ((index < N) && (states_[index] != occupied_state))
        {
            ++index;
        }
    }

    value_type* entry_ptr(size_type index) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<value_type>(slots_[index].address(0U));
    }

    CASTLE_CONST value_type* entry_ptr(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return memory::object_from_address<CASTLE_CONST value_type>(slots_[index].address(0U));
    }

    size_type size_;
    Hash hash_;
    KeyEqual equal_;
    uint8_t states_[N];
    memory::static_storage<value_type, 1U> slots_[N];
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_HASH_TABLE_H
