#include <gtest/gtest.h>

#include "castle/dsa/bloom_filter.hpp"
#include "castle/dsa/string_view.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::dsa::bloom_filter;
using castle::dsa::bloom_filter_for;
using castle::dsa::bloom_optimal_hash_count;

// Weak hasher: identity plus a seed. Shows that the filter works with poor user hashes and
// that seeded hashers change the bit layout.
struct seeded_hash
{
    size_type seed;

    size_type operator()(const uint32_t& key) const noexcept
    {
        return static_cast<size_type>(key) + seed;
    }
};

TEST(BloomFilterTest, OptimalHashCount)
{
    static_assert(bloom_optimal_hash_count(640U, 64U) == 7U, "10 bits per key");
    static_assert(bloom_optimal_hash_count(1024U, 100U) == 7U, "10.24 bits per key");
    static_assert(bloom_optimal_hash_count(1024U, 0U) == 1U, "zero keys");
    static_assert(bloom_optimal_hash_count(1U, 100U) == 1U, "rounds up to one");

    volatile size_type bits = 160U;
    volatile size_type keys = 10U;
    EXPECT_EQ(bloom_optimal_hash_count(bits, keys), 11U);      // 16 * ln 2 = 11.09
    EXPECT_EQ(bloom_optimal_hash_count(bits, 0U), 1U);
    EXPECT_EQ(bloom_optimal_hash_count(1U, keys), 1U);         // 0.0693 rounds to 0, clamped to 1
    EXPECT_EQ(bloom_optimal_hash_count(3U, 2U), 1U);           // 1.04
    EXPECT_EQ(bloom_optimal_hash_count(5U, 2U), 2U);           // 1.73
    EXPECT_EQ(bloom_optimal_hash_count(1U << 30U, 1U), 744260924U); // 64-bit math, no overflow
}

TEST(BloomFilterTest, SizedAliasPicksBitsAndHashCount)
{
    using filter_type = bloom_filter_for<uint32_t, 64U>;
    static_assert(filter_type::bit_size() == 640U, "64 keys * 10 bits");
    static_assert(filter_type::hash_count() == 7U, "round(10 * ln 2)");

    using dense_type = bloom_filter_for<uint32_t, 32U, 16U>;
    static_assert(dense_type::bit_size() == 512U, "32 keys * 16 bits");
    static_assert(dense_type::hash_count() == 11U, "round(16 * ln 2)");

    using explicit_type = bloom_filter<uint32_t, 640U, 7U>;
    static_assert(sizeof(filter_type) == sizeof(explicit_type), "same layout");
    EXPECT_EQ(filter_type::bitset_type::size(), 640U);
}

TEST(BloomFilterTest, StartsEmpty)
{
    bloom_filter<uint32_t, 256U, 4U> filter;
    EXPECT_TRUE(filter.empty());
    EXPECT_EQ(filter.bit_count(), 0U);
    EXPECT_FLOAT_EQ(filter.false_positive_rate(), 0.0F);
    EXPECT_TRUE(filter.bits().none());
    for (uint32_t key = 0U; key < 100U; ++key)
    {
        EXPECT_FALSE(filter.contains(key));
    }
}

TEST(BloomFilterTest, InsertReportsNewKeys)
{
    bloom_filter<uint32_t, 1024U, 5U> filter;
    EXPECT_TRUE(filter.insert(42U));
    EXPECT_FALSE(filter.empty());
    EXPECT_TRUE(filter.contains(42U));
    EXPECT_GE(filter.bit_count(), 1U);
    EXPECT_LE(filter.bit_count(), 5U);

    const size_type bits_after_first = filter.bit_count();
    EXPECT_FALSE(filter.insert(42U)); // every bit already set
    EXPECT_EQ(filter.bit_count(), bits_after_first);

    EXPECT_TRUE(filter.insert(7U));
    EXPECT_TRUE(filter.contains(7U));
    EXPECT_TRUE(filter.contains(42U));
}

TEST(BloomFilterTest, NoFalseNegativesPowerOfTwoBits)
{
    bloom_filter<uint32_t, 2048U, 7U> filter;
    for (uint32_t key = 0U; key < 200U; ++key)
    {
        filter.insert(key * 2654435761U);
    }
    for (uint32_t key = 0U; key < 200U; ++key)
    {
        EXPECT_TRUE(filter.contains(key * 2654435761U));
    }
    EXPECT_LE(filter.bit_count(), 200U * 7U);
}

TEST(BloomFilterTest, NoFalseNegativesOddBits)
{
    bloom_filter<uint32_t, 1999U, 7U> filter; // prime size: modulo path
    for (uint32_t key = 0U; key < 200U; ++key)
    {
        filter.insert(0x18DA0000U + key);
    }
    for (uint32_t key = 0U; key < 200U; ++key)
    {
        EXPECT_TRUE(filter.contains(0x18DA0000U + key));
    }
}

TEST(BloomFilterTest, FalsePositiveRateMatchesTheory)
{
    // n = 256, m = 2560, k = 7: theory gives about 0.82 % false positives.
    bloom_filter_for<uint32_t, 256U> filter;
    size_type added = 0U;
    for (uint32_t key = 0U; key < 256U; ++key)
    {
        if (filter.insert(key))
        {
            ++added;
        }
    }
    // A distinct key may already look present (a false positive at insert time).
    EXPECT_LT(added, 256U);
    EXPECT_GE(added, 250U);

    size_type false_positives = 0U;
    const uint32_t probes = 20000U;
    for (uint32_t key = 1000000U; key < 1000000U + probes; ++key)
    {
        if (filter.contains(key))
        {
            ++false_positives;
        }
    }
    const float measured = static_cast<float>(false_positives) / static_cast<float>(probes);
    EXPECT_GT(false_positives, 0U);
    EXPECT_LT(measured, 0.02F);

    // The fill-based estimate agrees with the measured rate.
    const float estimated = filter.false_positive_rate();
    EXPECT_GT(estimated, 0.004F);
    EXPECT_LT(estimated, 0.015F);
    EXPECT_NEAR(measured, estimated, 0.006F);
}

TEST(BloomFilterTest, SaturatedFilterAcceptsEverything)
{
    // One bit, one hash: every key maps to bit 0.
    bloom_filter<uint32_t, 1U, 1U> tiny;
    EXPECT_FALSE(tiny.contains(5U));
    EXPECT_TRUE(tiny.insert(5U));
    EXPECT_FALSE(tiny.insert(6U)); // false positive: bit already set
    EXPECT_TRUE(tiny.contains(123456U));
    EXPECT_EQ(tiny.bit_count(), 1U);
    EXPECT_FLOAT_EQ(tiny.false_positive_rate(), 1.0F);

    // HashCount == Bits: probes may revisit bits, the filter still never misses a key.
    bloom_filter<uint32_t, 3U, 3U> small;
    EXPECT_TRUE(small.insert(1U));
    EXPECT_TRUE(small.contains(1U));
    for (uint32_t key = 2U; key < 20U; ++key)
    {
        small.insert(key);
    }
    EXPECT_EQ(small.bit_count(), 3U);
    EXPECT_TRUE(small.bits().all());
    EXPECT_FLOAT_EQ(small.false_positive_rate(), 1.0F);
}

TEST(BloomFilterTest, FalsePositiveRateFollowsFill)
{
    bloom_filter<uint32_t, 64U, 2U> filter;
    filter.insert(1U);
    filter.insert(2U);
    const float fill = static_cast<float>(filter.bit_count()) / 64.0F;
    EXPECT_FLOAT_EQ(filter.false_positive_rate(), fill * fill);
}

TEST(BloomFilterTest, MergeIsUnion)
{
    bloom_filter<uint32_t, 512U, 4U> left;
    bloom_filter<uint32_t, 512U, 4U> right;
    bloom_filter<uint32_t, 512U, 4U> both;
    for (uint32_t key = 0U; key < 20U; ++key)
    {
        left.insert(key);
        both.insert(key);
    }
    for (uint32_t key = 100U; key < 120U; ++key)
    {
        right.insert(key);
        both.insert(key);
    }

    left.merge(right);
    EXPECT_TRUE(left.bits() == both.bits());
    for (uint32_t key = 0U; key < 20U; ++key)
    {
        EXPECT_TRUE(left.contains(key));
        EXPECT_TRUE(left.contains(key + 100U));
    }
}

TEST(BloomFilterTest, IntersectKeepsCommonKeys)
{
    bloom_filter<uint32_t, 4096U, 4U> left;
    bloom_filter<uint32_t, 4096U, 4U> right;
    for (uint32_t key = 0U; key < 30U; ++key)
    {
        left.insert(key);      // 0 .. 29
        right.insert(key + 20U); // 20 .. 49
    }

    left.intersect(right);
    for (uint32_t key = 20U; key < 30U; ++key)
    {
        EXPECT_TRUE(left.contains(key));
    }
    size_type survivors = 0U;
    for (uint32_t key = 0U; key < 20U; ++key)
    {
        if (left.contains(key))
        {
            ++survivors;
        }
    }
    EXPECT_LT(survivors, 3U); // keys of only one side almost always drop out

    bloom_filter<uint32_t, 4096U, 4U> disjoint;
    disjoint.insert(999U);
    left.intersect(disjoint);
    left.intersect(bloom_filter<uint32_t, 4096U, 4U>());
    EXPECT_TRUE(left.empty());
}

TEST(BloomFilterTest, ClearAndCopy)
{
    bloom_filter<uint32_t, 128U, 3U> filter{1U, 2U, 3U};
    EXPECT_TRUE(filter.contains(1U));
    EXPECT_TRUE(filter.contains(2U));
    EXPECT_TRUE(filter.contains(3U));

    bloom_filter<uint32_t, 128U, 3U> snapshot(filter);
    filter.clear();
    EXPECT_TRUE(filter.empty());
    EXPECT_EQ(filter.bit_count(), 0U);
    EXPECT_FALSE(filter.contains(1U));

    EXPECT_TRUE(snapshot.contains(2U));
    filter = snapshot;
    EXPECT_TRUE(filter.contains(3U));
    EXPECT_TRUE(filter.bits() == snapshot.bits());
}

TEST(BloomFilterTest, SeededHasherChangesLayout)
{
    using filter_type = bloom_filter<uint32_t, 1024U, 4U, seeded_hash>;
    filter_type first(seeded_hash{0U});
    filter_type second(seeded_hash{12345U});
    first.insert(77U);
    second.insert(77U);
    EXPECT_TRUE(first.contains(77U));
    EXPECT_TRUE(second.contains(77U));
    EXPECT_TRUE(first.bits() != second.bits());

    // An identity-like hash still yields a working filter.
    filter_type weak(seeded_hash{0U});
    for (uint32_t key = 0U; key < 64U; ++key)
    {
        weak.insert(key);
    }
    for (uint32_t key = 0U; key < 64U; ++key)
    {
        EXPECT_TRUE(weak.contains(key));
    }
    EXPECT_FALSE(weak.contains(5000U));
}

TEST(BloomFilterTest, StringViewKeys)
{
    using castle::dsa::string_view;
    bloom_filter_for<string_view, 16U> names;
    EXPECT_TRUE(names.insert(string_view("brake")));
    EXPECT_TRUE(names.insert(string_view("steering")));
    EXPECT_FALSE(names.insert(string_view("brake")));
    EXPECT_TRUE(names.contains(string_view("brake")));
    EXPECT_TRUE(names.contains(string_view("steering")));
    EXPECT_FALSE(names.contains(string_view("wiper")));
    EXPECT_FALSE(names.contains(string_view("")));
}

TEST(BloomFilterTest, SubsetAndEquality)
{
    using filter_type = bloom_filter<uint32_t, 256U, 4U>;
    filter_type empty_filter;
    filter_type part{1U, 2U};
    filter_type same_part{1U, 2U};
    filter_type whole{1U, 2U, 3U, 4U};

    EXPECT_TRUE(empty_filter.is_subset_of(empty_filter));
    EXPECT_TRUE(empty_filter.is_subset_of(part));
    EXPECT_TRUE(part.is_subset_of(whole));
    EXPECT_FALSE(whole.is_subset_of(part));
    EXPECT_TRUE(part == same_part);
    EXPECT_TRUE(part != whole);
    EXPECT_FALSE(part == whole);
}

TEST(BloomFilterTest, FullAndFillRatio)
{
    bloom_filter<uint32_t, 4U, 1U> small;
    EXPECT_TRUE(small.empty());
    EXPECT_FALSE(small.full());
    EXPECT_FLOAT_EQ(small.fill_ratio(), 0.0F);

    for (uint32_t key = 0U; key < 64U; ++key)
    {
        small.insert(key);
    }
    EXPECT_TRUE(small.full());
    EXPECT_FLOAT_EQ(small.fill_ratio(), 1.0F);
    EXPECT_FLOAT_EQ(small.false_positive_rate(), 1.0F);
}

TEST(BloomFilterTest, LoadWordsRestoresSeededFilter)
{
    using filter_type = bloom_filter<uint32_t, 128U, 3U, seeded_hash>;
    filter_type original(seeded_hash{99U});
    original.insert(11U);
    original.insert(22U);

    filter_type restored(seeded_hash{99U});
    EXPECT_EQ(restored.load_words(original.bits().data(), filter_type::word_count()), castle::status::ok);
    EXPECT_EQ(restored.hash_function().seed, original.hash_function().seed);
    EXPECT_TRUE(restored == original);
    EXPECT_TRUE(restored.contains(11U));
    EXPECT_TRUE(restored.contains(22U));
}

TEST(BloomFilterTest, LoadWordsRoundTripAndValidation)
{
    // Non-multiple of 32 bits: 37 bits uses 2 words, with 5 valid bits in the second word.
    using filter_type = bloom_filter<uint32_t, 37U, 3U>;
    static_assert(filter_type::word_count() == 2U, "37 bits needs 2 32-bit words");

    filter_type source{101U, 202U};
    filter_type restored;
    EXPECT_EQ(restored.load_words(nullptr, 2U), castle::status::invalid_argument);
    EXPECT_EQ(restored.load_words(source.bits().data(), 1U), castle::status::out_of_range);
    EXPECT_EQ(restored.load_words(source.bits().data(), 3U), castle::status::out_of_range);
    EXPECT_TRUE(restored.empty());

    EXPECT_EQ(restored.load_words(source.bits().data(), source.word_count()), castle::status::ok);
    EXPECT_TRUE(restored == source);
    EXPECT_TRUE(restored.contains(101U));
    EXPECT_TRUE(restored.contains(202U));

    // Dirty padding bits in the final word must be masked off to preserve bitset invariants.
    filter_type::word_type dirty[2U] = {0xFFFFFFFFU, 0xFFFFFFFFU};
    EXPECT_EQ(restored.load_words(dirty, 2U), castle::status::ok);
    EXPECT_EQ(restored.bit_count(), 37U);
    EXPECT_TRUE(restored.full());
    EXPECT_EQ(restored.bits().data()[1U], 0x1FU);

    // Exact multiple of 32 bits: 64 bits uses 2 full words with no masked bits.
    using aligned_type = bloom_filter<uint32_t, 64U, 3U>;
    aligned_type aligned;
    aligned_type::word_type full_words[2U] = {0xFFFFFFFFU, 0xFFFFFFFFU};
    EXPECT_EQ(aligned.load_words(full_words, 2U), castle::status::ok);
    EXPECT_EQ(aligned.bit_count(), 64U);
    EXPECT_TRUE(aligned.full());
}
} // namespace
