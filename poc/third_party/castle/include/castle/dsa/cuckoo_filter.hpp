// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cuckoo_filter.hpp
 * @brief Fixed-size cuckoo filter: a probabilistic set with deletion and no false negatives.
 *
 * Use `cuckoo_filter<Key, Buckets>` when a membership test must be cheap in RAM, an occasional
 * false "maybe present" answer is acceptable, and keys must also be removed again: tracking
 * active sessions or connections, de-duplicating frames inside a sliding window, caching which
 * flash records exist while records come and go, or blocking and unblocking IDs at runtime. Unlike
 * `dsa::bloom_filter`, a cuckoo filter supports `erase()` and `count()`.
 *
 * The table holds `Buckets` buckets of `BucketSize` fingerprint slots. A key maps to one
 * fingerprint `f` (the non-zero high bits of its hash) and two candidate buckets: `i1` from the
 * hash and `i2 = i1 XOR hash(f)` (partial-key cuckoo hashing, Fan et al.). Either bucket can
 * compute the other from `f` alone, so a full bucket can relocate ("kick") a stored fingerprint to
 * its alternate bucket. At most `MaxKicks` relocations bound the worst-case insert time. When they
 * all fail, `insert()` undoes them and returns `status::full`: no stored fingerprint is ever lost,
 * so `contains()` never answers `false` for a key that was inserted and not erased.
 *
 * With `b = BucketSize`, `f = 8 * sizeof(Fingerprint)` bits, and load factor `a`, the
 * false-positive rate is about `2 * b * a / (2^f - 1)`. `cuckoo_filter_for<Key, ExpectedKeys>`
 * sizes the table from the expected key count.
 *
 * @code
 * castle::dsa::cuckoo_filter_for<uint32_t, 100U> sessions; // 32 buckets x 4 slots of 16 bits
 * castle::status added = sessions.insert(0xC0A80017U);       // status::ok
 * bool maybe = sessions.contains(0xC0A80017U);               // true
 * castle::status removed = sessions.erase(0xC0A80017U);      // status::ok
 * @endcode
 */
#ifndef CASTLE_DSA_CUCKOO_FILTER_HPP
#define CASTLE_DSA_CUCKOO_FILTER_HPP

#include "castle/bit/bit_math.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/grid.hpp"
#include "castle/utility/hash.hpp"
#include "castle/utility/swap.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Returns the power-of-two bucket count that holds a key budget at most 90 % full.
 * @param expected_keys Number of keys the filter is designed for.
 * @param bucket_size Fingerprint slots per bucket. Must be positive.
 * @return `next_power_of_two(ceil(expected_keys / (0.9 * bucket_size)))`, at least 1.
 * @note Integer-only and `constexpr`, so it can feed the `Buckets` template argument. The 10 %
 * headroom keeps inserts fast and makes `status::full` unlikely before `expected_keys` keys.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR size_type cuckoo_bucket_count(size_type expected_keys, size_type bucket_size) CASTLE_NOEXCEPT
{
    CASTLE_CONST uint64_t slots = static_cast<uint64_t>(bucket_size) * 9U;
    CASTLE_CONST uint64_t buckets = (static_cast<uint64_t>(expected_keys) * 10U + slots - 1U) / slots;
    return bit::next_power_of_two(static_cast<size_type>(buckets));
}

/**
 * @brief Fixed-size cuckoo filter over `Key` values.
 * @tparam Key Key type passed to `Hash`.
 * @tparam Buckets Number of buckets. Must be a power of two.
 * @tparam BucketSize Fingerprint slots per bucket, `1 .. (2^f - 2) / 2`. 4 is the usual choice.
 * @tparam Fingerprint `uint8_t`, `uint16_t`, or `uint32_t`. Its width `f` sets the
 * false-positive rate. Zero marks an empty slot, so stored fingerprints are `1 .. 2^f - 1`.
 * @tparam MaxKicks Relocations tried before `insert()` gives up. Bounds the worst-case insert time.
 * @tparam Hash Hash functor returning `size_type`. Its `operator()` must be `const`.
 * @note Storage is one `grid<Fingerprint, Buckets, BucketSize>`, one size, and one `Hash` object.
 * The filter is copyable.
 * @note `contains()`, `count()`, and `erase()` are O(BucketSize). `insert()` is O(BucketSize)
 * while a candidate bucket has a free slot, and O(MaxKicks * BucketSize) in the worst case.
 * @warning Erase only keys that were inserted. Erasing a never-inserted key that collides with a
 * stored fingerprint removes that fingerprint and creates a false negative for its key.
 */
template <typename Key,
          size_type Buckets,
          size_type BucketSize = 4U,
          typename Fingerprint = uint16_t,
          size_type MaxKicks = 128U,
          typename Hash = castle::hash<Key>>
class cuckoo_filter
{
    static_assert(meta::is_power_of_two<Buckets>::value, "dsa::cuckoo_filter requires a power-of-two bucket count");
    static_assert(BucketSize > 0U, "dsa::cuckoo_filter requires at least one slot per bucket");
    static_assert(Buckets <= static_cast<castle::size_type>(-1) / 2U / BucketSize, "dsa::cuckoo_filter is too large");
    static_assert(meta::is_integral<Fingerprint>::value && meta::is_unsigned<Fingerprint>::value &&
                      !meta::is_same<Fingerprint, bool>::value && (sizeof(Fingerprint) <= 4U),
                  "dsa::cuckoo_filter requires uint8_t, uint16_t, or uint32_t fingerprints");
    static_assert(2U * BucketSize < static_cast<uint64_t>(castle::numeric_limits<Fingerprint>::max()),
                  "dsa::cuckoo_filter fingerprint is too short for the bucket size");

public:
    using key_type = Key;
    using fingerprint_type = Fingerprint;
    using hasher = Hash;
    using size_type = castle::size_type;
    using table_type = grid<Fingerprint, Buckets, BucketSize>;

    /** @brief Constructs an empty filter with a default-constructed hash functor. */
    cuckoo_filter() CASTLE_NOEXCEPT
        : table_(), size_(0U), hash_()
    {
    }

    /**
     * @brief Constructs an empty filter with a caller-supplied hash functor.
     * @param hash Hash functor copied into the filter, for example a seeded hasher.
     */
    explicit cuckoo_filter(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT
        : table_(), size_(0U), hash_(hash)
    {
    }

    /** @brief Returns the number of buckets. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type bucket_count() CASTLE_NOEXCEPT
    {
        return Buckets;
    }

    /** @brief Returns the number of fingerprint slots per bucket. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type bucket_size() CASTLE_NOEXCEPT
    {
        return BucketSize;
    }

    /** @brief Returns the number of fingerprint slots, `Buckets * BucketSize`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return Buckets * BucketSize;
    }

    /** @brief Returns the relocation budget of one `insert()`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type max_kicks() CASTLE_NOEXCEPT
    {
        return MaxKicks;
    }

    /** @brief Returns the number of stored fingerprints, `0 .. capacity()`. */
    CASTLE_NODISCARD size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /** @brief Reports whether no fingerprint is stored. */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

    /** @brief Returns the fraction of used slots, `size() / capacity()`, in `[0.0f, 1.0f]`. */
    CASTLE_NODISCARD float load_factor() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<float>(size_) / static_cast<float>(capacity());
    }

    /**
     * @brief Adds a key to the filter.
     * @param key Key to add. Adding a key again stores one more copy of its fingerprint.
     * @return `status::ok`, or `status::full` when no free slot was reached within `MaxKicks`
     * relocations. On `status::full` the filter is unchanged.
     * @note Complexity: O(BucketSize) with a free slot in a candidate bucket, otherwise up to
     * O(MaxKicks * BucketSize) including the undo on failure.
     */
    castle::status insert(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST location where = locate(key);
        if (place(where.first, where.fingerprint) || place(where.second, where.fingerprint))
        {
            ++size_;
            return castle::status::ok;
        }
        return relocate(where.first, where.fingerprint);
    }

    /**
     * @brief Adds a key only when `contains()` reports it absent.
     * @param key Key to add.
     * @return `status::ok`, `status::already_exists` when `key` was inserted before or is a false
     * positive, or `status::full` as for `insert()`. The filter is unchanged unless `status::ok`.
     * @note Keeps one fingerprint per key, so a later `erase()` removes the key completely.
     */
    castle::status insert_unique(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        if (contains(key))
        {
            return castle::status::already_exists;
        }
        return insert(key);
    }

    /**
     * @brief Tests whether a key may be in the filter.
     * @param key Key to test.
     * @return `false` when `key` is certainly absent. `true` when it was inserted and not erased,
     * or is a false positive.
     * @note Complexity: O(BucketSize).
     */
    CASTLE_NODISCARD bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST location where = locate(key);
        return (matches(where.first, where.fingerprint) != 0U) || (matches(where.second, where.fingerprint) != 0U);
    }

    /**
     * @brief Counts the stored copies of a key's fingerprint.
     * @param key Key to count.
     * @return Number of matching fingerprints in both candidate buckets: at least the number of
     * times `key` was inserted and not erased, more only through fingerprint collisions.
     * @note Complexity: O(BucketSize).
     */
    CASTLE_NODISCARD size_type count(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST location where = locate(key);
        CASTLE_CONST size_type first = matches(where.first, where.fingerprint);
        return (where.second == where.first) ? first : first + matches(where.second, where.fingerprint);
    }

    /**
     * @brief Removes one copy of a key's fingerprint.
     * @param key Key to remove. Must have been inserted; see the class warning.
     * @return `status::ok`, or `status::not_found` when no matching fingerprint is stored.
     * @note Complexity: O(BucketSize).
     */
    castle::status erase(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST location where = locate(key);
        if (remove(where.first, where.fingerprint) || remove(where.second, where.fingerprint))
        {
            --size_;
            return castle::status::ok;
        }
        return castle::status::not_found;
    }

    /** @brief Removes every fingerprint. Complexity: O(Buckets * BucketSize). */
    void clear() CASTLE_NOEXCEPT
    {
        table_.fill(empty_slot);
        size_ = 0U;
    }

    /**
     * @brief Estimates the current false-positive probability from the fill level.
     * @return About `2 * size() / (Buckets * (2^f - 1))`: a lookup compares the fingerprint with
     * `2 * BucketSize * load_factor()` stored ones, and each matches with probability `1 / (2^f - 1)`.
     */
    CASTLE_NODISCARD float false_positive_rate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (2.0F * static_cast<float>(size_)) /
               (static_cast<float>(Buckets) * static_cast<float>(castle::numeric_limits<Fingerprint>::max()));
    }

    /**
     * @brief Loads a raw fingerprint table, for example after a reset or a transfer.
     * @param fingerprints Source array of `capacity()` fingerprints in `table().data()` order.
     * @param count Number of fingerprints in `fingerprints`; must equal `capacity()`.
     * @return `status::ok`, `status::invalid_argument` when `fingerprints == nullptr`, or
     * `status::out_of_range` when `count != capacity()`. The filter is unchanged on failure.
     * @note `size()` becomes the number of non-zero fingerprints. Complexity: O(Buckets * BucketSize).
     */
    castle::status load_table(CASTLE_CONST fingerprint_type* fingerprints, size_type count) CASTLE_NOEXCEPT
    {
        if (fingerprints == nullptr)
        {
            return castle::status::invalid_argument;
        }
        if (count != capacity())
        {
            return castle::status::out_of_range;
        }

        fingerprint_type* dest = table_.data();
        size_ = 0U;
        for (size_type i = 0U; i < count; ++i)
        {
            dest[i] = fingerprints[i];
            if (fingerprints[i] != empty_slot)
            {
                ++size_;
            }
        }
        return castle::status::ok;
    }

    /**
     * @brief Returns the fingerprint table.
     * @return Read-only `Buckets x BucketSize` grid; zero marks a free slot. `table().data()`
     * exposes the fingerprints for persistence or transfer.
     */
    CASTLE_NODISCARD CASTLE_CONST table_type& table() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return table_;
    }

    /** @brief Returns the stored hash functor. */
    CASTLE_NODISCARD CASTLE_CONST hasher& hash_function() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash_;
    }

    /**
     * @brief Compares two filters slot by slot.
     * @note Equal key sets inserted in a different order can produce different layouts, and so
     * compare unequal. The hash functors are not compared.
     */
    CASTLE_NODISCARD bool operator==(CASTLE_CONST cuckoo_filter& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (size_ == other.size_) && (table_ == other.table_);
    }

    /** @brief Reports whether two filters differ in any slot. */
    CASTLE_NODISCARD bool operator!=(CASTLE_CONST cuckoo_filter& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    static CASTLE_CONSTEXPR fingerprint_type empty_slot = 0U;

    /** @brief Fingerprint and candidate buckets of one key. */
    struct location
    {
        fingerprint_type fingerprint;
        size_type first;
        size_type second;
    };

    location locate(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type hash_value = hash_(key);
        // The bucket uses the low hash bits; the fingerprint uses the high bits of an independent
        // remix, so the two stay uncorrelated even when `size_type` has only 32 bits.
        CASTLE_CONST uint64_t mixed = castle::detail::hash_mix64(static_cast<uint64_t>(hash_value));
        fingerprint_type fingerprint = static_cast<fingerprint_type>(mixed >> (64U - 8U * sizeof(fingerprint_type)));
        if (fingerprint == empty_slot)
        {
            fingerprint = 1U;
        }
        CASTLE_CONST size_type first = hash_value & (Buckets - 1U);
        return location{fingerprint, first, alternate(first, fingerprint)};
    }

    /** @brief Returns the other candidate bucket. Applying it twice returns `bucket`. */
    static size_type alternate(size_type bucket, fingerprint_type fingerprint) CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint64_t offset = castle::detail::hash_mix64(static_cast<uint64_t>(fingerprint));
        return bucket ^ (static_cast<size_type>(offset) & (Buckets - 1U));
    }

    /** @brief Returns the slot evicted at relocation `kick` in `bucket`; a pure function, so it can be undone. */
    static size_type victim_slot(size_type bucket, size_type kick) CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint32_t mixed = static_cast<uint32_t>(bucket + kick) * 2654435761U; // Knuth's golden-ratio multiplier
        return static_cast<size_type>(mixed >> 16U) % BucketSize;
    }

    bool place(size_type bucket, fingerprint_type fingerprint) CASTLE_NOEXCEPT
    {
        for (size_type slot = 0U; slot < BucketSize; ++slot)
        {
            fingerprint_type& cell = table_(bucket, slot);
            if (cell == empty_slot)
            {
                cell = fingerprint;
                return true;
            }
        }
        return false;
    }

    bool remove(size_type bucket, fingerprint_type fingerprint) CASTLE_NOEXCEPT
    {
        for (size_type slot = 0U; slot < BucketSize; ++slot)
        {
            fingerprint_type& cell = table_(bucket, slot);
            if (cell == fingerprint)
            {
                cell = empty_slot;
                return true;
            }
        }
        return false;
    }

    size_type matches(size_type bucket, fingerprint_type fingerprint) CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type found = 0U;
        for (size_type slot = 0U; slot < BucketSize; ++slot)
        {
            if (table_(bucket, slot) == fingerprint)
            {
                ++found;
            }
        }
        return found;
    }

    /**
     * @brief Kicks fingerprints along their alternate buckets until one lands in a free slot.
     * @note On failure, every swap is undone in reverse order. Each step is reversible without a
     * log: the previous bucket is `alternate(bucket, carried)` and the slot is `victim_slot()`.
     */
    castle::status relocate(size_type start, fingerprint_type fingerprint) CASTLE_NOEXCEPT
    {
        size_type bucket = start;
        fingerprint_type carried = fingerprint;
        for (size_type kick = 0U; kick < MaxKicks; ++kick)
        {
            castle::swap(carried, table_(bucket, victim_slot(bucket, kick)));
            bucket = alternate(bucket, carried);
            if (place(bucket, carried))
            {
                ++size_;
                return castle::status::ok;
            }
        }

        for (size_type kick = MaxKicks; kick > 0U; --kick)
        {
            bucket = alternate(bucket, carried);
            castle::swap(carried, table_(bucket, victim_slot(bucket, kick - 1U)));
        }
        return castle::status::full;
    }

    table_type table_;
    size_type size_;
    Hash hash_;
};

/**
 * @brief Cuckoo filter sized from the expected number of keys, with 4-slot buckets.
 * @tparam Key Key type passed to `Hash`.
 * @tparam ExpectedKeys Number of keys the filter is designed for.
 * @tparam Fingerprint Fingerprint type. `uint16_t` gives about 0.012 % false positives when full.
 * @tparam Hash Hash functor returning `size_type`.
 * @note Uses `Buckets = cuckoo_bucket_count(ExpectedKeys, 4)`, `BucketSize = 4`, and `MaxKicks = 128`.
 */
template <typename Key,
          size_type ExpectedKeys,
          typename Fingerprint = uint16_t,
          typename Hash = castle::hash<Key>>
using cuckoo_filter_for = cuckoo_filter<Key, cuckoo_bucket_count(ExpectedKeys, 4U), 4U, Fingerprint, 128U, Hash>;

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_CUCKOO_FILTER_HPP
