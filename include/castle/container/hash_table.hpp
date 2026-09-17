// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file hash_table.hpp
 * @brief Fixed-capacity open-addressed hash table with linear probing and tombstones.
 *
 * Use this container when deterministic hashed lookup is needed without dynamic allocation and
 * the maximum number of entries is known at compile time. The table stores up to `N` key/value
 * pairs in embedded storage, probes linearly for at most `N` slots, and marks erased slots as
 * tombstones so later lookups and insertions remain correct until `clear()` resets the states.
 * Average successful operations are typically O(1); worst-case lookup, insertion, and erasure
 * are O(N).
 *
 * @note Existing elements never move because there is no rehashing or dynamic growth.
 * @warning Capacity is fixed at compile time; insertions return `status::full` when all probe
 * positions are exhausted.
 */
#ifndef CASTLE_CONTAINER_HASH_TABLE_HPP
#define CASTLE_CONTAINER_HASH_TABLE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/iterator/tags.hpp"
#include "castle/error/status.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/utility/compare.hpp"
#include "castle/utility/hash.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/pair.hpp"
#include <stdint.h>

namespace castle
{
namespace container
{

/**
 * @brief Deterministic fixed-capacity hash table with linear probing.
 * @tparam Key Key type.
 * @tparam T Mapped value type.
 * @tparam N Number of hash slots and maximum number of stored entries.
 * @tparam Hash Hash functor used to map keys to buckets.
 * @tparam KeyEqual Equality functor used to compare keys.
 * @note Erased entries leave tombstones until `clear()` so probe chains remain intact.
 * @warning The table is non-copyable and non-movable because it owns in-place entry storage.
 */
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

    /**
     * @brief Forward iterator over occupied table entries.
     * @note Increment skips empty and deleted slots.
     */
    class iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = hash_table::value_type;
        using difference_type = castle::difference_type;
        using pointer = value_type*;
        using reference = value_type&;

        /**
         * @brief Constructs an end iterator.
         */
        iterator() CASTLE_NOEXCEPT : table_(nullptr), index_(0U) {}

        /**
         * @brief Constructs an iterator over a specific table slot.
         * @param table Owning table.
         * @param index Slot index.
         */
        iterator(hash_table* table, size_type index) CASTLE_NOEXCEPT : table_(table), index_(index) {}

        /**
         * @brief Dereferences the current occupied entry.
         * @return Reference to the key/value pair stored at the current slot.
         * @warning The iterator must not equal `end()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *table_->entry_ptr(index_); }

        /**
         * @brief Returns the address of the current occupied entry.
         * @return Pointer to the key/value pair stored at the current slot.
         * @warning The iterator must not equal `end()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return table_->entry_ptr(index_); }

        /**
         * @brief Advances to the next occupied slot.
         * @return Reference to `*this`.
         * @note Complexity is O(1) amortized and O(N) worst case.
         */
        iterator& operator++() CASTLE_NOEXCEPT
        {
            if (table_ != nullptr)
            {
                table_->advance_index(index_);
            }
            return *this;
        }

        /**
         * @brief Returns the current iterator and then advances.
         * @return Iterator state before incrementing.
         */
        iterator operator++(int) CASTLE_NOEXCEPT { iterator temp(*this); ++(*this); return temp; }

        /**
         * @brief Tests whether two iterators reference the same table slot.
         * @param other Iterator to compare with.
         * @return `true` when both iterators reference the same table and slot.
         */
        bool operator==(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return table_ == other.table_ && index_ == other.index_; } // LCOV_EXCL_BR_LINE

        /**
         * @brief Tests whether two iterators reference different table slots.
         * @param other Iterator to compare with.
         * @return `true` when the iterators differ.
         */
        bool operator!=(CASTLE_CONST iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

        /**
         * @brief Converts to a const iterator.
         * @return Const iterator referencing the same slot.
         */
        operator const_iterator() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(table_, index_); }

    private:
        friend class hash_table;
        friend class const_iterator;
        hash_table* table_;
        size_type index_;
    };

    /**
     * @brief Forward iterator over occupied table entries with const access.
     * @note Increment skips empty and deleted slots.
     */
    class const_iterator
    {
    public:
        using iterator_category = forward_iterator_tag;
        using value_type = hash_table::value_type;
        using difference_type = castle::difference_type;
        using pointer = CASTLE_CONST value_type*;
        using reference = CASTLE_CONST value_type&;

        /**
         * @brief Constructs an end iterator.
         */
        const_iterator() CASTLE_NOEXCEPT : table_(nullptr), index_(0U) {}

        /**
         * @brief Constructs an iterator over a specific table slot.
         * @param table Owning table.
         * @param index Slot index.
         */
        const_iterator(CASTLE_CONST hash_table* table, size_type index) CASTLE_NOEXCEPT : table_(table), index_(index) {}

        /**
         * @brief Constructs a const iterator from a mutable iterator.
         * @param other Mutable iterator to copy.
         */
        const_iterator(CASTLE_CONST iterator& other) CASTLE_NOEXCEPT : table_(other.table_), index_(other.index_) {}

        /**
         * @brief Dereferences the current occupied entry.
         * @return Const reference to the key/value pair stored at the current slot.
         * @warning The iterator must not equal `end()`.
         */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *table_->entry_ptr(index_); }

        /**
         * @brief Returns the address of the current occupied entry.
         * @return Const pointer to the key/value pair stored at the current slot.
         * @warning The iterator must not equal `end()`.
         */
        pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return table_->entry_ptr(index_); }

        /**
         * @brief Advances to the next occupied slot.
         * @return Reference to `*this`.
         */
        const_iterator& operator++() CASTLE_NOEXCEPT
        {
            if (table_ != nullptr)
            {
                table_->advance_index(index_);
            }
            return *this;
        }

        /**
         * @brief Returns the current iterator and then advances.
         * @return Iterator state before incrementing.
         */
        const_iterator operator++(int) CASTLE_NOEXCEPT { const_iterator temp(*this); ++(*this); return temp; }

        /**
         * @brief Tests whether two iterators reference the same table slot.
         * @param other Iterator to compare with.
         * @return `true` when both iterators reference the same table and slot.
         */
        bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return table_ == other.table_ && index_ == other.index_; } // LCOV_EXCL_BR_LINE

        /**
         * @brief Tests whether two iterators reference different table slots.
         * @param other Iterator to compare with.
         * @return `true` when the iterators differ.
         */
        bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT { return !(*this == other); }

    private:
        friend class hash_table;
        CASTLE_CONST hash_table* table_;
        size_type index_;
    };

    /**
     * @brief Constructs an empty table with default hash and equality functors.
     * @note Complexity is O(1).
     */
    hash_table() CASTLE_NOEXCEPT : size_(0U), hash_(), equal_(), states_{}
    {
    }

    /**
     * @brief Constructs an empty table with a caller-supplied hash functor.
     * @param hash Hash functor used for bucket selection.
     * @note Complexity is O(1).
     */
    explicit hash_table(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT
        : size_(0U), hash_(hash), equal_(), states_{}
    {
    }

    /**
     * @brief Constructs a table by inserting entries from an initializer list.
     * @param list Source entries.
     * @note Complexity is O(list.size()) average and O(list.size() * N) worst case.
     * Duplicate keys after the first are ignored.
     * @warning Oversized initializer lists trigger `CASTLE_ASSERT`.
     */
    hash_table(initializer_list<value_type> list) CASTLE_NOEXCEPT
        : size_(0U), hash_(), equal_(), states_{}
    {
        for (CASTLE_CONST value_type& entry : list)
        {
            if (full())
            {
                break;
            }
            insert(entry.first, entry.second);
        }
    }

    hash_table(CASTLE_CONST hash_table&) CASTLE_DELETE;
    hash_table& operator=(CASTLE_CONST hash_table&) CASTLE_DELETE;
    hash_table(hash_table&&) CASTLE_DELETE;
    hash_table& operator=(hash_table&&) CASTLE_DELETE;

    /**
     * @brief Destroys all occupied entries.
     * @note Complexity is O(N).
     */
    ~hash_table() CASTLE_NOEXCEPT { clear(); }

    /**
     * @brief Returns the fixed compile-time slot count.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the number of occupied entries.
     * @return Current entry count.
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the number of free entry slots.
     * @return `capacity() - size()`.
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    /**
     * @brief Tests whether the table contains no entries.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Tests whether all slots are occupied.
     * @return `true` when `size() == capacity()`.
     * @note Deleted slots count as available after reinsertion.
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Returns an iterator to the first occupied slot.
     * @return Iterator to the first stored entry or `end()` when the table is empty.
     */
    iterator begin() CASTLE_NOEXCEPT { return iterator(this, first_occupied()); }

    /**
     * @brief Returns a const iterator to the first occupied slot.
     * @return Const iterator to the first stored entry or `end()` when the table is empty.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, first_occupied()); }

    /**
     * @brief Returns the past-the-end iterator.
     * @return Iterator whose slot index equals `N`.
     */
    iterator end() CASTLE_NOEXCEPT { return iterator(this, N); }

    /**
     * @brief Returns the past-the-end const iterator.
     * @return Const iterator whose slot index equals `N`.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, N); }

    /**
     * @brief Returns a const iterator to the first occupied slot.
     * @return Const iterator equal to `begin()`.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns the past-the-end const iterator.
     * @return Const iterator equal to `end()`.
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /**
     * @brief Inserts a new key/value pair by copy.
     * @param key Key to insert.
     * @param value Value to associate with `key`.
     * @return `status::ok` on success, `status::already_exists` when `key` is already present, or
     * `status::full` when no slot can accept the entry.
     * @note Average complexity is O(1); worst case is O(N).
     */
    status insert(CASTLE_CONST Key& key, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return insert_value(key, value);
    }

    /**
     * @brief Inserts a new key/value pair by move.
     * @param key Key to insert.
     * @param value Value to associate with `key`.
     * @return `status::ok` on success, `status::already_exists` when `key` is already present, or
     * `status::full` when no slot can accept the entry.
     * @note Average complexity is O(1); worst case is O(N).
     */
    status insert(CASTLE_CONST Key& key, T&& value) CASTLE_NOEXCEPT
    {
        return insert_value(key, CASTLE_MOVE(value));
    }

    /**
     * @brief Constructs a mapped value in place and inserts it under `key`.
     * @tparam Args Constructor argument types for `T`.
     * @param key Key to insert.
     * @param args Arguments forwarded to `T`.
     * @return `status::ok` on success, `status::already_exists` when `key` is already present, or
     * `status::full` when no slot can accept the entry.
     * @note Average complexity is O(1); worst case is O(N).
     */
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

    /**
     * @brief Inserts a key only when it is absent.
     * @tparam Args Constructor argument types for `T`.
     * @param key Key to insert.
     * @param args Arguments forwarded to `T`.
     * @return Same status values as `emplace()`.
     * @note Complexity and semantics match `emplace()`.
     */
    template <typename... Args>
    status try_emplace(CASTLE_CONST Key& key, Args&&... args) CASTLE_NOEXCEPT
    {
        return emplace(key, CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Inserts a new entry or overwrites the mapped value of an existing key.
     * @param key Key to insert or update.
     * @param value Replacement mapped value.
     * @return `status::ok` on success or `status::full` when insertion into an absent key finds
     * no available slot.
     * @note Average complexity is O(1); worst case is O(N).
     */
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

    /**
     * @brief Inserts a new entry or overwrites the mapped value of an existing key.
     * @param key Key to insert or update.
     * @param value Replacement mapped value.
     * @return `status::ok` on success or `status::full` when insertion into an absent key finds
     * no available slot.
     * @note Average complexity is O(1); worst case is O(N).
     */
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

    /**
     * @brief Finds an entry by key.
     * @param key Key to search for.
     * @return Iterator to the matching entry or `end()` when absent.
     * @note Average complexity is O(1); worst case is O(N).
     */
    iterator find(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        return iterator(this, find_index(key));
    }

    /**
     * @brief Finds an entry by key.
     * @param key Key to search for.
     * @return Const iterator to the matching entry or `end()` when absent.
     * @note Average complexity is O(1); worst case is O(N).
     */
    const_iterator find(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_iterator(this, find_index(key));
    }

    /**
     * @brief Tests whether `key` is present.
     * @param key Key to search for.
     * @return `true` when a matching entry exists.
     * @note Average complexity is O(1); worst case is O(N).
     */
    bool contains(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_index(key) != N;
    }

    /**
     * @brief Returns a pointer to the mapped value of `key`.
     * @param key Key to search for.
     * @return Pointer to the mapped value or `nullptr` when the key is absent.
     * @note Average complexity is O(1); worst case is O(N).
     */
    mapped_type* get(CASTLE_CONST Key& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(key);
        return index == N
               ? nullptr
               : &entry_ptr(index)->second;
    }

    /**
     * @brief Returns a pointer to the mapped value of `key`.
     * @param key Key to search for.
     * @return Const pointer to the mapped value or `nullptr` when the key is absent.
     * @note Average complexity is O(1); worst case is O(N).
     */
    CASTLE_CONST mapped_type* get(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(key);
        return index == N
               ? nullptr
               : &entry_ptr(index)->second;
    }

    /**
     * @brief Erases the entry whose key equals `key`.
     * @param key Key to erase.
     * @return `status::ok` on success or `status::not_found` when no such key exists.
     * @note Average complexity is O(1); worst case is O(N). Erasure leaves a tombstone so later
     * probes still traverse the original collision chain correctly.
     */
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

    /**
     * @brief Destroys all occupied entries and resets all slot states to empty.
     * @note Complexity is O(N).
     */
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

    /**
     * @brief Inserts a key-value pair into the hash table.
     * @param key Key to insert.
     * @param value Value to associate with the key.
     * @return `status::ok` on success, `status::already_exists` if the key is already present, or `status::full` if the table is full.
     * @note Average complexity is O(1); worst case is O(N).
     */
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
                if (first_deleted == N) // LCOV_EXCL_BR_LINE
                {
                    // Reuse the first tombstone only if no truly empty slot appears sooner.
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

    /**
     * @brief Computes the bucket index for the given key.
     * @param key Key for which to compute the bucket index.
     * @return Bucket index for the given key.
     */
    size_type bucket(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash_(key) % N;
    }

    /**
     * @brief Finds the index of the given key in the hash table.
     * @param key Key to search for.
     * @return Index of the key if found, or `N` if not found.
     * @note Complexity is O(N) in the worst case.
     */
    size_type find_index(CASTLE_CONST Key& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type start = bucket(key);
        for (size_type step = 0U; step < N; ++step)
        {
            CASTLE_CONST size_type index = (start + step) % N;
            if (states_[index] == empty_state)
            {
                // Linear probing can stop at the first never-used slot: the key cannot appear later.
                return N;
            }
            if ((states_[index] == occupied_state) && equal_(entry_ptr(index)->first, key))
            {
                return index;
            }
        }
        return N;
    }

    /**
     * @brief Finds the index of the first occupied slot in the hash table.
     * @return Index of the first occupied slot, or `N` if no slots are occupied.
     */
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

    /**
     * @brief Advances the given index to the next occupied slot in the hash table.
     * @param index Index to advance. After the call, it will point to the next occupied slot or `N` if no more occupied slots exist.
     */
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

    /**
     * @brief Retrieves a pointer to the entry at the given index in the hash table.
     * @param index Index of the entry to retrieve.
     * @return Pointer to the entry at the given index, or `nullptr` if the index is out of bounds.
     */
    value_type* entry_ptr(size_type index) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<value_type>(slots_[index].address(0U));
    }

    /**
     * @brief Retrieves a pointer to the entry at the given index in the hash table (const version).
     * @param index Index of the entry to retrieve.
     * @return Pointer to the entry at the given index, or `nullptr` if the index is out of bounds.
     */
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

#endif // CASTLE_CONTAINER_HASH_TABLE_HPP
