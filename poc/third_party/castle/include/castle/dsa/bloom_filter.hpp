// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bloom_filter.hpp
 * @brief Fixed-size Bloom filter: a probabilistic set with no false negatives.
 *
 * Use `bloom_filter<Key, Bits, HashCount>` when a membership test must be cheap in RAM and an
 * occasional false "maybe present" answer is acceptable: guarding slow flash or EEPROM lookups,
 * dropping duplicate CAN or network frames, rejecting unknown IDs before a full table search, or
 * exchanging compact set summaries between nodes. The filter stores `Bits` bits and no keys, so
 * keys cannot be enumerated or erased.
 *
 * `contains()` never returns `false` for an inserted key. It returns `true` for an absent key with
 * probability close to `(1 - e^(-k*n/m))^k`, where `m = Bits`, `k = HashCount`, and `n` is the
 * number of distinct inserted keys. `bloom_filter_for<Key, ExpectedKeys, BitsPerKey>` sizes the
 * filter from the expected key count and picks the optimal `k = round(BitsPerKey * ln 2)`.
 *
 * Bit positions come from one `Hash` call per operation through enhanced double hashing
 * (Dillinger and Manolios): `x = h1 mod m`, `y = h2 mod m`, then `x += y; y += i` for each next
 * probe, where `h2` is a remix of `h1`. Every operation costs O(HashCount) with no division
 * inside the probe loop.
 *
 * @code
 * castle::dsa::bloom_filter_for<uint32_t, 64U> seen; // 640 bits, 7 probes, about 0.8 % false positives
 * seen.insert(0x18DAF110U);
 * bool maybe = seen.contains(0x18DAF110U); // true
 * bool never = seen.contains(0x7DFU);      // false, except for a rare false positive
 * @endcode
 */
#ifndef CASTLE_DSA_BLOOM_FILTER_HPP
#define CASTLE_DSA_BLOOM_FILTER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/status.hpp"
#include "castle/dsa/initializer_list.hpp"
#include "castle/math/powi.hpp"
#include "castle/utility/bitset.hpp"
#include "castle/utility/hash.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Returns the hash count that minimizes false positives for a bit and key budget.
 * @param bits Filter size `m` in bits.
 * @param expected_keys Number `n` of distinct keys the filter is designed for.
 * @return `round(m / n * ln 2)`, at least 1. Returns 1 when `expected_keys == 0`.
 * @note Integer-only and `constexpr`, so it can feed the `HashCount` template argument.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR size_type bloom_optimal_hash_count(size_type bits, size_type expected_keys) CASTLE_NOEXCEPT
{
    if (expected_keys == 0U)
    {
        return 1U;
    }

    // ln 2 ~= 693147 / 1000000; 64-bit products keep large bit budgets exact.
    CASTLE_CONST uint64_t denominator = static_cast<uint64_t>(expected_keys) * 1000000ULL;
    CASTLE_CONST uint64_t numerator = static_cast<uint64_t>(bits) * 693147ULL + denominator / 2U;
    CASTLE_CONST size_type count = static_cast<size_type>(numerator / denominator);
    return (count == 0U) ? 1U : count;
}

/**
 * @brief Fixed-size Bloom filter over `Key` values.
 * @tparam Key Key type passed to `Hash`.
 * @tparam Bits Filter size `m` in bits. Must be greater than zero.
 * @tparam HashCount Number `k` of bit positions per key, `1 .. Bits`.
 * @tparam Hash Hash functor returning `size_type`. Its `operator()` must be `const`.
 * @note Storage is one `castle::bitset<Bits>` plus one `Hash` object. The filter is copyable.
 * @note `insert()` and `contains()` are O(HashCount); `merge()`, `intersect()`, `is_subset_of()`,
 * `load_words()`, `clear()`, `empty()`, `full()`, `bit_count()`, and `operator==` are O(Bits / 32).
 * @warning Keys cannot be removed: clearing one key's bits would create false negatives for
 * other keys. Use `clear()` to start over.
 */
template <typename Key,
          size_type Bits,
          size_type HashCount,
          typename Hash = castle::hash<Key>>
class bloom_filter
{
    static_assert(Bits > 0U, "dsa::bloom_filter requires a positive bit count");
    static_assert(Bits <= static_cast<castle::size_type>(-1) / 2U, "dsa::bloom_filter bit count is too large");
    static_assert(HashCount > 0U, "dsa::bloom_filter requires at least one hash");
    static_assert(HashCount <= Bits, "dsa::bloom_filter hash count must not exceed the bit count");

public:
    using key_type = Key;
    using hasher = Hash;
    using size_type = castle::size_type;
    using bitset_type = castle::bitset<Bits>;
    using word_type = typename bitset_type::word_type;

    /**
     * @brief Constructs an empty filter with a default-constructed hash functor.
     * @note Complexity: O(Bits / 32).
     */
    bloom_filter() CASTLE_NOEXCEPT
        : bits_(), hash_()
    {
    }

    /**
     * @brief Constructs an empty filter with a caller-supplied hash functor.
     * @param hash Hash functor copied into the filter, for example a seeded hasher.
     * @note Complexity: O(Bits / 32).
     */
    explicit bloom_filter(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT
        : bits_(), hash_(hash)
    {
    }

    /**
     * @brief Constructs a filter holding every key of a list.
     * @param list Keys inserted in list order.
     * @note Complexity: O(list.size() * HashCount).
     */
    bloom_filter(initializer_list<key_type> list) CASTLE_NOEXCEPT
        : bits_(), hash_()
    {
        for (CASTLE_CONST key_type& key : list)
        {
            insert(key);
        }
    }

    /** @brief Returns the filter size `m` in bits. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type bit_size() CASTLE_NOEXCEPT
    {
        return Bits;
    }

    /** @brief Returns the number `k` of bit positions set per key. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type hash_count() CASTLE_NOEXCEPT
    {
        return HashCount;
    }

    /** @brief Returns the number of storage words in `bits().data()`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type word_count() CASTLE_NOEXCEPT
    {
        return bitset_type::number_of_words();
    }

    /**
     * @brief Adds a key to the filter.
     * @param key Key to add.
     * @return `true` when at least one bit changed, so `key` was certainly absent before.
     * `false` when every bit was already set: `key` was inserted before or is a false positive.
     * @note Complexity: O(HashCount).
     */
    bool insert(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        bool added = false;
        probe position(hash_(key));
        for (size_type i = 0U; i < HashCount; ++i)
        {
            if (!bits_.test(position.index()))
            {
                bits_.set(position.index());
                added = true;
            }
            position.advance();
        }
        return added;
    }

    /**
     * @brief Tests whether a key may be in the filter.
     * @param key Key to test.
     * @return `false` when `key` was certainly never inserted. `true` when it was inserted or is a
     * false positive.
     * @note Complexity: O(HashCount); stops at the first clear bit.
     */
    CASTLE_NODISCARD bool contains(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        probe position(hash_(key));
        for (size_type i = 0U; i < HashCount; ++i)
        {
            if (!bits_.test(position.index()))
            {
                return false;
            }
            position.advance();
        }
        return true;
    }

    /**
     * @brief Adds every key of another filter (set union).
     * @param other Filter of the same type built with an equivalent hash functor.
     * @note The result equals a filter that received the keys of both filters. Complexity: O(Bits / 32).
     * @warning Filters with different hash seeds produce a meaningless union.
     */
    void merge(CASTLE_CONST bloom_filter& other) CASTLE_NOEXCEPT
    {
        bits_ |= other.bits_;
    }

    /**
     * @brief Keeps only the bits set in both filters (approximate set intersection).
     * @param other Filter of the same type built with an equivalent hash functor.
     * @note Keys inserted into both filters still test `true`. The false-positive rate may be higher
     * than that of a filter built from the true intersection. Complexity: O(Bits / 32).
     * @warning Filters with different hash seeds produce a meaningless intersection.
     */
    void intersect(CASTLE_CONST bloom_filter& other) CASTLE_NOEXCEPT
    {
        bits_ &= other.bits_;
    }

    /**
     * @brief Reports whether every set bit of `*this` is also set in `other`.
     * @param other Candidate superset filter built with an equivalent hash functor.
     * @return `true` when `(bits_ & ~other.bits_) == 0`; `false` otherwise.
     * @note Complexity: O(Bits / 32), zero temporary storage.
     */
    CASTLE_NODISCARD bool is_subset_of(CASTLE_CONST bloom_filter& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST word_type* lhs = bits_.data();
        CASTLE_CONST word_type* rhs = other.bits_.data();
        for (size_type i = 0U; i < word_count(); ++i)
        {
            if ((lhs[i] & static_cast<word_type>(~rhs[i])) != static_cast<word_type>(0))
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Loads raw storage words into the filter, masking off unused trailing bits.
     * @param words Source word array containing `word_count()` words.
     * @param count Number of words in `words`; must equal `word_count()`.
     * @return `status::ok`, `status::invalid_argument` when `words == nullptr`, or
     * `status::out_of_range` when `count != word_count()`. Filter is unchanged on failure.
     * @note Complexity: O(Bits / 32).
     */
    castle::status load_words(CASTLE_CONST word_type* words, size_type count) CASTLE_NOEXCEPT
    {
        if (words == nullptr)
        {
            return castle::status::invalid_argument;
        }
        if (count != word_count())
        {
            return castle::status::out_of_range;
        }

        word_type* dest = bits_.data();
        for (size_type i = 0U; i < count; ++i)
        {
            dest[i] = words[i];
        }
        // Clear the padding bits past `Bits`: `bitset::count()` and `bitset::all()` rely on them being zero.
        CASTLE_CONST size_type padding = word_count() * bitset_type::bits_per_word() - Bits;
        dest[count - 1U] &= static_cast<word_type>(static_cast<word_type>(~static_cast<word_type>(0)) >> padding);
        return castle::status::ok;
    }

    /** @brief Removes every key by clearing all bits. */
    void clear() CASTLE_NOEXCEPT
    {
        bits_.reset();
    }

    /** @brief Reports whether every bit is clear, so `contains()` returns `false` for any key. */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_.none();
    }

    /** @brief Reports whether every bit is set, so `contains()` returns `true` for any key. */
    CASTLE_NODISCARD bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_.all();
    }

    /** @brief Returns the number of set bits, `0 .. Bits`. */
    CASTLE_NODISCARD size_type bit_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_.count();
    }

    /**
     * @brief Returns the fraction of set bits in `[0.0f, 1.0f]`.
     * @note Complexity: O(Bits / 32).
     */
    CASTLE_NODISCARD float fill_ratio() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<float>(bits_.count()) / static_cast<float>(Bits);
    }

    /**
     * @brief Estimates the current false-positive probability from the fill level.
     * @return `(bit_count() / Bits) ^ HashCount`, in `[0, 1]`.
     * @note Uses the real fill level, so it also covers duplicates and merged filters.
     * Complexity: O(Bits / 32 + log HashCount).
     */
    CASTLE_NODISCARD float false_positive_rate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return math::powi(fill_ratio(), static_cast<unsigned>(HashCount));
    }

    /**
     * @brief Returns the underlying bit storage.
     * @return Read-only bitset; `bits().data()` exposes the words for persistence or transfer.
     */
    CASTLE_NODISCARD CASTLE_CONST bitset_type& bits() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_;
    }

    /** @brief Returns the stored hash functor. */
    CASTLE_NODISCARD CASTLE_CONST hasher& hash_function() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash_;
    }

    /**
     * @brief Compares two filters by their bit states.
     * @note The hash functors are not compared; check `hash_function()` when seeds may differ.
     */
    CASTLE_NODISCARD bool operator==(CASTLE_CONST bloom_filter& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_ == other.bits_;
    }

    /** @brief Reports whether two filters have different bit states. */
    CASTLE_NODISCARD bool operator!=(CASTLE_CONST bloom_filter& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_ != other.bits_;
    }

private:
    using probe = castle::detail::double_hash_probe<Bits>;

    bitset_type bits_;
    Hash hash_;
};

/**
 * @brief Bloom filter sized from the expected number of keys.
 * @tparam Key Key type passed to `Hash`.
 * @tparam ExpectedKeys Number `n` of distinct keys the filter is designed for. Must be positive.
 * @tparam BitsPerKey Bits `m / n` per expected key. 10 gives about 0.8 % false positives.
 * @tparam Hash Hash functor returning `size_type`.
 * @note Uses `Bits = ExpectedKeys * BitsPerKey` and `HashCount = bloom_optimal_hash_count(Bits, ExpectedKeys)`.
 */
template <typename Key,
          size_type ExpectedKeys,
          size_type BitsPerKey = 10U,
          typename Hash = castle::hash<Key>>
using bloom_filter_for = bloom_filter<Key,
                                      ExpectedKeys * BitsPerKey,
                                      bloom_optimal_hash_count(ExpectedKeys * BitsPerKey, ExpectedKeys),
                                      Hash>;

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_BLOOM_FILTER_HPP
