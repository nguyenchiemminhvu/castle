#include <gtest/gtest.h>

#include "castle/dsa/cuckoo_filter.hpp"
#include "castle/dsa/string_view.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;
using castle::dsa::cuckoo_bucket_count;
using castle::dsa::cuckoo_filter;
using castle::dsa::cuckoo_filter_for;

// Weak hasher: identity plus a seed. The first bucket of key `k` is then `(k + seed) mod Buckets`,
// which lets tests put several keys into the same bucket on purpose.
struct seeded_hash
{
    size_type seed;

    size_type operator()(const uint32_t& key) const noexcept
    {
        return static_cast<size_type>(key) + seed;
    }
};

// Spreads test keys over the 32-bit range.
uint32_t spread(uint32_t index)
{
    return index * 2654435761U + 12345U;
}

// Fingerprint of key `k` under `seeded_hash{0}`, mirroring the filter: the top bits of
// `hash_mix64(k)`, with zero replaced by one.
template <typename Fingerprint>
Fingerprint fingerprint_of(uint32_t key)
{
    const uint64_t mixed = castle::detail::hash_mix64(static_cast<uint64_t>(key));
    const Fingerprint value = static_cast<Fingerprint>(mixed >> (64U - 8U * sizeof(Fingerprint)));
    return (value == 0U) ? static_cast<Fingerprint>(1U) : value;
}

template <typename Filter>
size_type stored_fingerprints(const Filter& filter)
{
    size_type used = 0U;
    for (const typename Filter::fingerprint_type cell : filter.table())
    {
        if (cell != 0U)
        {
            ++used;
        }
    }
    return used;
}

TEST(CuckooFilterTest, BucketCountSizing)
{
    static_assert(cuckoo_bucket_count(100U, 4U) == 32U, "100 / 3.6 = 27.8, next power of two");
    static_assert(cuckoo_bucket_count(36U, 4U) == 16U, "exactly 10 buckets at 90 %, rounded up");
    static_assert(cuckoo_bucket_count(0U, 4U) == 1U, "zero keys still need one bucket");
    static_assert(cuckoo_bucket_count(1U, 4U) == 1U, "one key fits one bucket");

    volatile size_type keys = 1000U;
    volatile size_type slots = 2U;
    EXPECT_EQ(cuckoo_bucket_count(keys, 4U), 512U);   // 277.8 buckets
    EXPECT_EQ(cuckoo_bucket_count(keys, slots), 1024U); // 555.6 buckets
    EXPECT_EQ(cuckoo_bucket_count(9U, 1U), 16U);        // exactly 10 buckets
}

TEST(CuckooFilterTest, SizedAliasPicksGeometry)
{
    using filter_type = cuckoo_filter_for<uint32_t, 100U>;
    static_assert(filter_type::bucket_count() == 32U, "power-of-two buckets");
    static_assert(filter_type::bucket_size() == 4U, "four slots per bucket");
    static_assert(filter_type::capacity() == 128U, "32 * 4 slots");
    static_assert(filter_type::max_kicks() == 128U, "default relocation budget");

    using explicit_type = cuckoo_filter<uint32_t, 32U, 4U, uint16_t, 128U>;
    static_assert(sizeof(filter_type) == sizeof(explicit_type), "same layout");

    using small_type = cuckoo_filter_for<uint32_t, 100U, uint8_t>;
    static_assert(sizeof(small_type::table_type) == 128U, "one byte per slot");
}

TEST(CuckooFilterTest, StartsEmpty)
{
    cuckoo_filter<uint32_t, 16U> filter;
    EXPECT_TRUE(filter.empty());
    EXPECT_EQ(filter.size(), 0U);
    EXPECT_FLOAT_EQ(filter.load_factor(), 0.0F);
    EXPECT_FLOAT_EQ(filter.false_positive_rate(), 0.0F);
    EXPECT_EQ(stored_fingerprints(filter), 0U);
    for (uint32_t i = 0U; i < 200U; ++i)
    {
        EXPECT_FALSE(filter.contains(spread(i)));
        EXPECT_EQ(filter.count(spread(i)), 0U);
    }
    EXPECT_EQ(filter.erase(7U), status::not_found);
    EXPECT_TRUE(filter.empty());
}

TEST(CuckooFilterTest, InsertContainsEraseRoundTrip)
{
    cuckoo_filter_for<uint32_t, 100U> filter;
    for (uint32_t i = 0U; i < 100U; ++i)
    {
        ASSERT_EQ(filter.insert(spread(i)), status::ok);
    }
    EXPECT_EQ(filter.size(), 100U);
    EXPECT_EQ(stored_fingerprints(filter), 100U);
    EXPECT_FLOAT_EQ(filter.load_factor(), 100.0F / 128.0F);
    EXPECT_FALSE(filter.empty());

    // No false negatives.
    for (uint32_t i = 0U; i < 100U; ++i)
    {
        EXPECT_TRUE(filter.contains(spread(i)));
        EXPECT_GE(filter.count(spread(i)), 1U);
    }

    // Erase the even keys: the odd keys stay present.
    for (uint32_t i = 0U; i < 100U; i += 2U)
    {
        EXPECT_EQ(filter.erase(spread(i)), status::ok);
    }
    EXPECT_EQ(filter.size(), 50U);
    for (uint32_t i = 1U; i < 100U; i += 2U)
    {
        EXPECT_TRUE(filter.contains(spread(i)));
    }

    for (uint32_t i = 1U; i < 100U; i += 2U)
    {
        EXPECT_EQ(filter.erase(spread(i)), status::ok);
    }
    EXPECT_TRUE(filter.empty());
    EXPECT_EQ(stored_fingerprints(filter), 0U);
    for (uint32_t i = 0U; i < 100U; ++i)
    {
        EXPECT_FALSE(filter.contains(spread(i)));
    }
}

TEST(CuckooFilterTest, DuplicatesAreCounted)
{
    cuckoo_filter<uint32_t, 64U> filter;
    const uint32_t key = 0x18DAF110U;
    EXPECT_EQ(filter.insert(key), status::ok);
    EXPECT_EQ(filter.insert(key), status::ok);
    EXPECT_EQ(filter.insert(key), status::ok);
    EXPECT_EQ(filter.count(key), 3U);
    EXPECT_EQ(filter.size(), 3U);

    EXPECT_EQ(filter.erase(key), status::ok);
    EXPECT_EQ(filter.count(key), 2U);
    EXPECT_TRUE(filter.contains(key));
    EXPECT_EQ(filter.erase(key), status::ok);
    EXPECT_EQ(filter.erase(key), status::ok);
    EXPECT_FALSE(filter.contains(key));
    EXPECT_EQ(filter.erase(key), status::not_found);
    EXPECT_TRUE(filter.empty());
}

TEST(CuckooFilterTest, InsertUniqueKeepsOneCopy)
{
    cuckoo_filter<uint32_t, 64U> filter;
    EXPECT_EQ(filter.insert_unique(42U), status::ok);
    EXPECT_EQ(filter.insert_unique(42U), status::already_exists);
    EXPECT_EQ(filter.count(42U), 1U);
    EXPECT_EQ(filter.size(), 1U);

    // One erase then removes the key completely.
    EXPECT_EQ(filter.erase(42U), status::ok);
    EXPECT_FALSE(filter.contains(42U));
    EXPECT_EQ(filter.insert_unique(42U), status::ok);
}

TEST(CuckooFilterTest, InsertUniqueReportsFull)
{
    cuckoo_filter<uint32_t, 1U, 2U> filter; // a single bucket of two slots
    EXPECT_EQ(filter.insert_unique(1U), status::ok);
    EXPECT_EQ(filter.insert_unique(2U), status::ok);
    EXPECT_EQ(filter.insert_unique(3U), status::full);
    EXPECT_EQ(filter.size(), 2U);
}

TEST(CuckooFilterTest, SingleBucketCountsEachSlotOnce)
{
    // With one bucket, both candidate buckets are bucket 0: count() must not count it twice.
    cuckoo_filter<uint32_t, 1U, 4U> filter;
    EXPECT_EQ(filter.insert(9U), status::ok);
    EXPECT_EQ(filter.insert(9U), status::ok);
    EXPECT_EQ(filter.count(9U), 2U);
    EXPECT_EQ(filter.insert(9U), status::ok);
    EXPECT_EQ(filter.insert(9U), status::ok);
    EXPECT_EQ(filter.count(9U), 4U);
    EXPECT_FLOAT_EQ(filter.load_factor(), 1.0F);

    // Every slot is used: relocation stays in the same bucket and fails, leaving the filter intact.
    const cuckoo_filter<uint32_t, 1U, 4U> before = filter;
    EXPECT_EQ(filter.insert(10U), status::full);
    EXPECT_TRUE(filter == before);
    EXPECT_EQ(filter.count(9U), 4U);
}

TEST(CuckooFilterTest, SecondBucketIsUsedWhenFirstIsFull)
{
    // Identity hash: keys 3, 3 + 16, 3 + 32, ... all start in bucket 3.
    using filter_type = cuckoo_filter<uint32_t, 16U, 2U, uint16_t, 16U, seeded_hash>;
    filter_type filter(seeded_hash{0U});
    ASSERT_EQ(filter.insert(3U), status::ok);
    ASSERT_EQ(filter.insert(19U), status::ok);
    EXPECT_EQ(filter.table()(3U, 0U), fingerprint_of<uint16_t>(3U));
    EXPECT_EQ(filter.table()(3U, 1U), fingerprint_of<uint16_t>(19U));

    // Bucket 3 is full; key 35 goes to its alternate bucket.
    const uint32_t key = 35U;
    ASSERT_EQ(filter.insert(key), status::ok);
    EXPECT_EQ(filter.table()(3U, 0U), fingerprint_of<uint16_t>(3U));
    EXPECT_EQ(filter.table()(3U, 1U), fingerprint_of<uint16_t>(19U));
    EXPECT_TRUE(filter.contains(key));
    EXPECT_EQ(filter.count(key), 1U);

    // Erasing it searches the first bucket, then removes it from the second.
    EXPECT_EQ(filter.erase(key), status::ok);
    EXPECT_FALSE(filter.contains(key));
    EXPECT_TRUE(filter.contains(3U));
    EXPECT_TRUE(filter.contains(19U));
    EXPECT_EQ(filter.size(), 2U);
}

TEST(CuckooFilterTest, RelocationRaisesTheReachableLoad)
{
    // Without relocation, an insert fails as soon as both candidate buckets are full.
    cuckoo_filter<uint32_t, 64U, 4U, uint16_t, 0U> no_kicks;
    uint32_t without = 0U;
    while (no_kicks.insert(spread(without)) == status::ok)
    {
        ++without;
    }

    cuckoo_filter<uint32_t, 64U, 4U, uint16_t, 128U> kicks;
    uint32_t with = 0U;
    while (kicks.insert(spread(with)) == status::ok)
    {
        ++with;
    }

    EXPECT_GT(with, without);
    EXPECT_GT(kicks.load_factor(), 0.9F);
    EXPECT_EQ(kicks.size(), with);
    EXPECT_EQ(stored_fingerprints(kicks), with);

    // Relocated fingerprints are still found through their alternate bucket.
    for (uint32_t i = 0U; i < with; ++i)
    {
        EXPECT_TRUE(kicks.contains(spread(i)));
    }
}

TEST(CuckooFilterTest, FailedInsertUndoesEveryKick)
{
    cuckoo_filter<uint32_t, 8U, 2U, uint16_t, 32U> filter;
    uint32_t inserted = 0U;
    cuckoo_filter<uint32_t, 8U, 2U, uint16_t, 32U> before;
    for (;;)
    {
        before = filter;
        if (filter.insert(spread(inserted)) != status::ok)
        {
            break;
        }
        ++inserted;
    }

    // The failed insert kicked 32 fingerprints around, then put every one back.
    EXPECT_TRUE(filter == before);
    EXPECT_FALSE(filter != before);
    EXPECT_EQ(filter.size(), inserted);
    for (uint32_t i = 0U; i < inserted; ++i)
    {
        EXPECT_TRUE(filter.contains(spread(i)));
    }

    // Erasing one key makes room again.
    EXPECT_EQ(filter.erase(spread(0U)), status::ok);
    EXPECT_EQ(filter.insert(spread(0U)), status::ok);
}

TEST(CuckooFilterTest, ZeroFingerprintIsRemapped)
{
    // Find a key whose raw 8-bit fingerprint is zero, the empty-slot marker.
    uint32_t key = 0U;
    while ((castle::detail::hash_mix64(static_cast<uint64_t>(key)) >> 56U) != 0U)
    {
        ++key;
    }

    using filter_type = cuckoo_filter<uint32_t, 16U, 4U, uint8_t, 16U, seeded_hash>;
    filter_type filter(seeded_hash{0U});
    EXPECT_FALSE(filter.contains(key)); // a zero fingerprint would match every free slot
    EXPECT_EQ(filter.insert(key), status::ok);
    EXPECT_TRUE(filter.contains(key));
    EXPECT_EQ(filter.table()(key & 15U, 0U), 1U);
    EXPECT_EQ(filter.erase(key), status::ok);
    EXPECT_FALSE(filter.contains(key));
}

TEST(CuckooFilterTest, FalsePositiveRateMatchesTheEstimate)
{
    cuckoo_filter<uint32_t, 256U, 4U, uint8_t> filter;
    uint32_t inserted = 0U;
    while (filter.load_factor() < 0.9F)
    {
        ASSERT_EQ(filter.insert(spread(inserted)), status::ok);
        ++inserted;
    }
    // About 2 * 4 * 0.9 / 255 = 2.8 % for 8-bit fingerprints.
    const float estimate = filter.false_positive_rate();
    EXPECT_NEAR(estimate, 2.0F * static_cast<float>(inserted) / (256.0F * 255.0F), 1e-6F);

    uint32_t hits = 0U;
    const uint32_t probes = 20000U;
    for (uint32_t i = 0U; i < probes; ++i)
    {
        if (filter.contains(spread(1000000U + i)))
        {
            ++hits;
        }
    }
    const float measured = static_cast<float>(hits) / static_cast<float>(probes);
    EXPECT_GT(measured, estimate * 0.6F);
    EXPECT_LT(measured, estimate * 1.4F);

    // 16-bit fingerprints: 256 times fewer false positives for the same load.
    cuckoo_filter<uint32_t, 256U, 4U, uint16_t> wide;
    for (uint32_t i = 0U; i < inserted; ++i)
    {
        ASSERT_EQ(wide.insert(spread(i)), status::ok);
    }
    EXPECT_LT(wide.false_positive_rate(), 0.0002F);
}

TEST(CuckooFilterTest, OddBucketSizeAndWideFingerprints)
{
    cuckoo_filter<uint32_t, 32U, 3U, uint32_t, 64U> filter;
    for (uint32_t i = 0U; i < 80U; ++i)
    {
        ASSERT_EQ(filter.insert(spread(i)), status::ok);
    }
    for (uint32_t i = 0U; i < 80U; ++i)
    {
        EXPECT_TRUE(filter.contains(spread(i)));
    }
    // 32-bit fingerprints: a false positive among 1000 probes is practically impossible.
    for (uint32_t i = 0U; i < 1000U; ++i)
    {
        EXPECT_FALSE(filter.contains(spread(5000U + i)));
    }
}

TEST(CuckooFilterTest, LoadTableRestoresAFilter)
{
    using filter_type = cuckoo_filter<uint32_t, 16U>;
    filter_type source;
    for (uint32_t i = 0U; i < 40U; ++i)
    {
        ASSERT_EQ(source.insert(spread(i)), status::ok);
    }

    filter_type restored;
    EXPECT_EQ(restored.load_table(nullptr, filter_type::capacity()), status::invalid_argument);
    EXPECT_EQ(restored.load_table(source.table().data(), filter_type::capacity() - 1U), status::out_of_range);
    EXPECT_TRUE(restored.empty());

    EXPECT_EQ(restored.load_table(source.table().data(), filter_type::capacity()), status::ok);
    EXPECT_TRUE(restored == source);
    EXPECT_EQ(restored.size(), 40U);
    for (uint32_t i = 0U; i < 40U; ++i)
    {
        EXPECT_TRUE(restored.contains(spread(i)));
    }
}

TEST(CuckooFilterTest, ClearAndEquality)
{
    cuckoo_filter<uint32_t, 16U> lhs;
    cuckoo_filter<uint32_t, 16U> rhs;
    EXPECT_TRUE(lhs == rhs);

    ASSERT_EQ(lhs.insert(1U), status::ok);
    EXPECT_TRUE(lhs != rhs); // sizes differ

    ASSERT_EQ(rhs.insert(2U), status::ok);
    EXPECT_FALSE(lhs == rhs); // same size, different slots

    ASSERT_EQ(rhs.erase(2U), status::ok);
    ASSERT_EQ(rhs.insert(1U), status::ok);
    EXPECT_TRUE(lhs == rhs); // same insert sequence, same layout

    lhs.clear();
    EXPECT_TRUE(lhs.empty());
    EXPECT_FALSE(lhs.contains(1U));
    EXPECT_EQ(stored_fingerprints(lhs), 0U);
    const cuckoo_filter<uint32_t, 16U> fresh;
    EXPECT_TRUE(lhs == fresh);
}

TEST(CuckooFilterTest, SeededHashersChangeTheLayout)
{
    using filter_type = cuckoo_filter<uint32_t, 16U, 4U, uint16_t, 128U, seeded_hash>;
    filter_type lhs(seeded_hash{0U});
    filter_type rhs(seeded_hash{5U});
    EXPECT_EQ(lhs.hash_function().seed, 0U);
    EXPECT_EQ(rhs.hash_function().seed, 5U);

    ASSERT_EQ(lhs.insert(100U), status::ok);
    ASSERT_EQ(rhs.insert(100U), status::ok);
    EXPECT_TRUE(lhs.contains(100U));
    EXPECT_TRUE(rhs.contains(100U));
    EXPECT_TRUE(lhs != rhs);
}

TEST(CuckooFilterTest, StringViewKeys)
{
    using castle::dsa::string_view;
    cuckoo_filter<string_view, 8U> sessions;
    EXPECT_EQ(sessions.insert(string_view("client-a")), status::ok);
    EXPECT_EQ(sessions.insert(string_view("client-b")), status::ok);
    EXPECT_TRUE(sessions.contains(string_view("client-a")));
    EXPECT_FALSE(sessions.contains(string_view("client-c")));
    EXPECT_EQ(sessions.erase(string_view("client-a")), status::ok);
    EXPECT_FALSE(sessions.contains(string_view("client-a")));
    EXPECT_TRUE(sessions.contains(string_view("client-b")));
}

} // namespace
