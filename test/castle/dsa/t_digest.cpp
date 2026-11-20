#include <gtest/gtest.h>

#include "castle/dsa/t_digest.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;
using castle::dsa::t_digest;

// Every value 0 .. 10006 exactly once, in a scrambled order (7919 is coprime with the prime 10007).
constexpr uint32_t stream_size = 10007U;

uint32_t scrambled(uint32_t index)
{
    return (index * 7919U) % stream_size;
}

template <typename Digest>
void feed_uniform(Digest& digest, uint32_t first = 0U, uint32_t last = stream_size)
{
    for (uint32_t i = first; i < last; ++i)
    {
        ASSERT_EQ(digest.add(static_cast<typename Digest::value_type>(scrambled(i))), status::ok);
    }
}

// Heavy right tail, like a jitter amplitude: x = 100 * u^4 for u uniform in [0, 1).
float skewed(uint32_t value)
{
    const float u = static_cast<float>(value) / static_cast<float>(stream_size);
    return 100.0F * u * u * u * u;
}

template <typename Digest>
void expect_sorted_and_complete(const Digest& digest)
{
    uint64_t weight = 0U;
    for (const typename Digest::centroid_type& centroid : digest)
    {
        EXPECT_GE(centroid.weight, 1U);
        weight += centroid.weight;
    }
    EXPECT_EQ(weight, digest.total());
    for (const typename Digest::centroid_type* it = digest.begin(); (it != digest.end()) && (it + 1 != digest.end()); ++it)
    {
        EXPECT_LE(it->mean, (it + 1)->mean);
    }
}

TEST(TDigestTest, Geometry)
{
    using digest_type = t_digest<float, 64U>;
    static_assert(digest_type::capacity() == 64U, "centroid capacity");
    static_assert(digest_type::buffer_size() == 64U, "buffer defaults to the capacity");
    EXPECT_FLOAT_EQ(digest_type::compression(), 63.0F);
    EXPECT_EQ(digest_type::max_weight(), UINT32_MAX);

    using small_type = t_digest<double, 2U, 1U>;
    static_assert(small_type::buffer_size() == 1U, "explicit buffer");
    EXPECT_DOUBLE_EQ(small_type::compression(), 1.0);
}

TEST(TDigestTest, StartsEmpty)
{
    t_digest<float, 16U> digest;
    EXPECT_TRUE(digest.empty());
    EXPECT_EQ(digest.total(), 0U);
    EXPECT_EQ(digest.size(), 0U);
    EXPECT_TRUE(digest.begin() == digest.end());
    EXPECT_FLOAT_EQ(digest.quantile(0.5F), 0.0F);
    EXPECT_FLOAT_EQ(digest.cdf(1.0F), 0.0F);
    EXPECT_FLOAT_EQ(digest.mean(), 0.0F);
    EXPECT_FLOAT_EQ(digest.min(), 0.0F);
    EXPECT_FLOAT_EQ(digest.max(), 0.0F);
}

TEST(TDigestTest, FewSamplesAreExact)
{
    // Ten samples stay ten singleton centroids: quantiles interpolate the sorted samples.
    t_digest<float, 64U> digest;
    const float samples[] = {7.0F, 3.0F, 10.0F, 1.0F, 5.0F, 9.0F, 2.0F, 8.0F, 4.0F, 6.0F};
    for (const float sample : samples)
    {
        ASSERT_EQ(digest.add(sample), status::ok);
    }
    EXPECT_EQ(digest.total(), 10U);
    EXPECT_FLOAT_EQ(digest.min(), 1.0F);
    EXPECT_FLOAT_EQ(digest.max(), 10.0F);
    EXPECT_FLOAT_EQ(digest.mean(), 5.5F);

    EXPECT_FLOAT_EQ(digest.quantile(0.0F), 1.0F);
    EXPECT_FLOAT_EQ(digest.quantile(0.03F), 1.0F);  // rank 0.3 lies between (0, min) and the first center
    EXPECT_FLOAT_EQ(digest.quantile(0.25F), 3.0F);  // rank 2.5 is the center of the third sample
    EXPECT_FLOAT_EQ(digest.quantile(0.5F), 5.5F);   // midway between 5 and 6
    EXPECT_FLOAT_EQ(digest.quantile(0.98F), 10.0F); // rank 9.8, between the last center and (10, max)
    EXPECT_FLOAT_EQ(digest.quantile(1.0F), 10.0F);
    EXPECT_EQ(digest.size(), 10U);
    expect_sorted_and_complete(digest);

    EXPECT_FLOAT_EQ(digest.cdf(0.5F), 0.0F);
    EXPECT_FLOAT_EQ(digest.cdf(1.0F), 0.05F);  // half of the sample at 1
    EXPECT_FLOAT_EQ(digest.cdf(5.0F), 0.45F);  // four samples below, half of the one at 5
    EXPECT_FLOAT_EQ(digest.cdf(5.5F), 0.5F);
    EXPECT_FLOAT_EQ(digest.cdf(10.0F), 1.0F);
    EXPECT_FLOAT_EQ(digest.cdf(11.0F), 1.0F);
}

TEST(TDigestTest, QuantileArgumentIsClamped)
{
    t_digest<float, 16U> digest;
    ASSERT_EQ(digest.add(2.0F), status::ok);
    ASSERT_EQ(digest.add(4.0F), status::ok);
    EXPECT_FLOAT_EQ(digest.quantile(-1.0F), 2.0F);
    EXPECT_FLOAT_EQ(digest.quantile(2.0F), 4.0F);
    EXPECT_FLOAT_EQ(digest.quantile(__builtin_nanf("")), 2.0F);
    EXPECT_FLOAT_EQ(digest.cdf(__builtin_nanf("")), 0.0F);
}

TEST(TDigestTest, AddRejectsNonFiniteValues)
{
    t_digest<float, 16U> digest;
    ASSERT_EQ(digest.add(1.0F), status::ok);
    EXPECT_EQ(digest.add(__builtin_nanf("")), status::invalid_argument);
    EXPECT_EQ(digest.add(__builtin_inff()), status::invalid_argument);
    EXPECT_EQ(digest.add(-__builtin_inff()), status::invalid_argument);
    EXPECT_EQ(digest.total(), 1U);
    EXPECT_FLOAT_EQ(digest.max(), 1.0F);

    // A zero weight is accepted and changes nothing.
    EXPECT_EQ(digest.add(50.0F, 0U), status::ok);
    EXPECT_EQ(digest.total(), 1U);
    EXPECT_FLOAT_EQ(digest.max(), 1.0F);
}

TEST(TDigestTest, TotalWeightIsBounded)
{
    t_digest<float, 16U> digest;
    ASSERT_EQ(digest.add(1.0F, UINT32_MAX - 1U), status::ok);
    ASSERT_EQ(digest.add(2.0F), status::ok);
    EXPECT_EQ(digest.total(), UINT32_MAX);
    EXPECT_EQ(digest.add(3.0F), status::full);
    EXPECT_EQ(digest.total(), UINT32_MAX);
    EXPECT_FLOAT_EQ(digest.max(), 2.0F); // unchanged on failure
    EXPECT_FLOAT_EQ(digest.quantile(0.5F), 1.0F);
}

TEST(TDigestTest, WeightedSamples)
{
    t_digest<double, 16U> weighted;
    t_digest<double, 16U> repeated;
    ASSERT_EQ(weighted.add(5.0, 3U), status::ok);
    ASSERT_EQ(weighted.add(1.0), status::ok);
    for (uint32_t i = 0U; i < 3U; ++i)
    {
        ASSERT_EQ(repeated.add(5.0), status::ok);
    }
    ASSERT_EQ(repeated.add(1.0), status::ok);

    EXPECT_EQ(weighted.total(), 4U);
    EXPECT_DOUBLE_EQ(weighted.mean(), 4.0);
    EXPECT_DOUBLE_EQ(weighted.cdf(1.0), 0.125);
    EXPECT_DOUBLE_EQ(repeated.cdf(1.0), 0.125);
    EXPECT_DOUBLE_EQ(weighted.quantile(0.75), 5.0);
    EXPECT_DOUBLE_EQ(repeated.quantile(0.75), 5.0);
}

TEST(TDigestTest, MinAndMaxAreExact)
{
    t_digest<float, 8U, 4U> digest;
    ASSERT_EQ(digest.add(5.0F), status::ok);
    ASSERT_EQ(digest.add(3.0F), status::ok); // new minimum
    ASSERT_EQ(digest.add(9.0F), status::ok); // new maximum
    ASSERT_EQ(digest.add(4.0F), status::ok); // inside the range
    EXPECT_FLOAT_EQ(digest.min(), 3.0F);
    EXPECT_FLOAT_EQ(digest.max(), 9.0F);
    feed_uniform(digest);
    EXPECT_FLOAT_EQ(digest.min(), 0.0F);
    EXPECT_FLOAT_EQ(digest.max(), 10006.0F);
    EXPECT_FLOAT_EQ(digest.quantile(0.0F), 0.0F);
    EXPECT_FLOAT_EQ(digest.quantile(1.0F), 10006.0F);
}

TEST(TDigestTest, UniformStreamQuantiles)
{
    t_digest<float, 64U> digest;
    feed_uniform(digest);
    digest.compress();
    EXPECT_EQ(digest.total(), stream_size);
    EXPECT_LE(digest.size(), digest.capacity());
    expect_sorted_and_complete(digest);
    EXPECT_NEAR(digest.mean(), 5003.0F, 0.5F);

    const float max_value = static_cast<float>(stream_size - 1U);
    const float qs[] = {0.01F, 0.05F, 0.25F, 0.5F, 0.68F, 0.95F, 0.99F, 0.999F};
    for (const float q : qs)
    {
        EXPECT_NEAR(digest.quantile(q), q * max_value, 0.005F * max_value) << "q = " << q;
        EXPECT_NEAR(digest.cdf(q * max_value), q, 0.005F) << "q = " << q;
    }
}

TEST(TDigestTest, SkewedStreamKeepsTheTailAccurate)
{
    t_digest<float, 64U> digest;
    for (uint32_t i = 0U; i < stream_size; ++i)
    {
        ASSERT_EQ(digest.add(skewed(scrambled(i))), status::ok);
    }
    // The t-digest bounds the rank error: the true fraction of samples below the estimate.
    const float qs[] = {0.5F, 0.68F, 0.95F, 0.99F, 0.999F};
    for (const float q : qs)
    {
        const float estimate = digest.quantile(q);
        uint32_t below = 0U;
        for (uint32_t value = 0U; value < stream_size; ++value)
        {
            below += (skewed(value) <= estimate) ? 1U : 0U;
        }
        EXPECT_NEAR(static_cast<float>(below) / static_cast<float>(stream_size), q, 0.005F) << "q = " << q;
    }
    // The tail is narrow in rank, so p95 and p99 are also close in value.
    const float p95 = skewed(static_cast<uint32_t>(0.95F * static_cast<float>(stream_size - 1U)));
    const float p99 = skewed(static_cast<uint32_t>(0.99F * static_cast<float>(stream_size - 1U)));
    EXPECT_NEAR(digest.quantile(0.95F), p95, 0.01F * p95);
    EXPECT_NEAR(digest.quantile(0.99F), p99, 0.01F * p99);
}

TEST(TDigestTest, CompressIsIdempotent)
{
    t_digest<float, 32U, 16U> digest;
    feed_uniform(digest, 0U, 1000U);
    digest.compress();
    const size_type size = digest.size();
    const float p90 = digest.quantile(0.9F);
    digest.compress();
    EXPECT_EQ(digest.size(), size);
    EXPECT_FLOAT_EQ(digest.quantile(0.9F), p90);
    expect_sorted_and_complete(digest);
}

TEST(TDigestTest, MeanIncludesBufferedSamples)
{
    t_digest<float, 8U, 64U> digest;
    ASSERT_EQ(digest.add(1.0F), status::ok);
    ASSERT_EQ(digest.add(2.0F), status::ok);
    ASSERT_EQ(digest.add(6.0F), status::ok);
    EXPECT_EQ(digest.size(), 3U); // all still buffered
    EXPECT_FLOAT_EQ(digest.mean(), 3.0F);
}

TEST(TDigestTest, TinyDigestStillBoundsTheExtremes)
{
    t_digest<float, 2U, 1U> digest;
    feed_uniform(digest);
    digest.compress();
    EXPECT_LE(digest.size(), 2U);
    expect_sorted_and_complete(digest);
    EXPECT_FLOAT_EQ(digest.quantile(0.0F), 0.0F);
    EXPECT_FLOAT_EQ(digest.quantile(1.0F), 10006.0F);
    // One centroid in the middle: the upper half interpolates out to the maximum.
    const float upper = digest.quantile(0.9F);
    EXPECT_GT(upper, digest.begin()->mean);
    EXPECT_LT(upper, 10006.0F);
    const float share = digest.cdf(9000.0F);
    EXPECT_GT(share, 0.5F);
    EXPECT_LT(share, 1.0F);
}

TEST(TDigestTest, MergeCombinesStreams)
{
    t_digest<float, 64U> lower;
    t_digest<float, 64U> upper;
    t_digest<float, 64U> whole;
    feed_uniform(lower, 0U, 5000U);
    feed_uniform(upper, 5000U, stream_size);
    feed_uniform(whole);

    t_digest<float, 64U> merged;
    EXPECT_EQ(merged.merge(lower), status::ok); // into an empty digest
    EXPECT_EQ(merged.merge(upper), status::ok);
    EXPECT_EQ(merged.total(), stream_size);
    EXPECT_FLOAT_EQ(merged.min(), whole.min());
    EXPECT_FLOAT_EQ(merged.max(), whole.max());
    const float qs[] = {0.05F, 0.5F, 0.95F, 0.99F};
    for (const float q : qs)
    {
        EXPECT_NEAR(merged.quantile(q), whole.quantile(q), 0.005F * 10006.0F) << "q = " << q;
    }

    // Merging an empty digest changes nothing.
    const float p50 = merged.quantile(0.5F);
    EXPECT_EQ(merged.merge(t_digest<float, 64U>()), status::ok);
    EXPECT_EQ(merged.total(), stream_size);
    EXPECT_FLOAT_EQ(merged.quantile(0.5F), p50);
}

TEST(TDigestTest, MergeKeepsTheExtremes)
{
    t_digest<float, 16U> middle;
    t_digest<float, 16U> wide;
    t_digest<float, 16U> narrow;
    ASSERT_EQ(middle.add(5.0F), status::ok);
    ASSERT_EQ(middle.add(6.0F), status::ok);
    ASSERT_EQ(wide.add(1.0F), status::ok);
    ASSERT_EQ(wide.add(9.0F), status::ok);
    ASSERT_EQ(narrow.add(5.5F), status::ok);

    EXPECT_EQ(middle.merge(narrow), status::ok); // extremes unchanged
    EXPECT_FLOAT_EQ(middle.min(), 5.0F);
    EXPECT_FLOAT_EQ(middle.max(), 6.0F);
    EXPECT_EQ(middle.merge(wide), status::ok); // both extremes move
    EXPECT_FLOAT_EQ(middle.min(), 1.0F);
    EXPECT_FLOAT_EQ(middle.max(), 9.0F);
    EXPECT_EQ(middle.total(), 5U);
}

TEST(TDigestTest, MergeWithItselfDoublesTheWeights)
{
    t_digest<float, 32U> digest;
    feed_uniform(digest, 0U, 2000U);
    const float p25 = digest.quantile(0.25F);
    const float p75 = digest.quantile(0.75F);
    EXPECT_EQ(digest.merge(digest), status::ok);
    EXPECT_EQ(digest.total(), 4000U);
    EXPECT_FLOAT_EQ(digest.quantile(0.25F), p25);
    EXPECT_FLOAT_EQ(digest.quantile(0.75F), p75);
    expect_sorted_and_complete(digest);
}

TEST(TDigestTest, MergeReportsFull)
{
    t_digest<float, 16U> big;
    t_digest<float, 16U> small;
    ASSERT_EQ(big.add(1.0F, UINT32_MAX), status::ok);
    ASSERT_EQ(small.add(2.0F), status::ok);
    EXPECT_EQ(small.merge(big), status::full);
    EXPECT_EQ(small.total(), 1U);
    EXPECT_FLOAT_EQ(small.min(), 2.0F);
    EXPECT_EQ(big.merge(big), status::full);
    EXPECT_EQ(big.total(), UINT32_MAX);
}

TEST(TDigestTest, ReplayingCentroidsRestoresTheDigest)
{
    t_digest<float, 32U> source;
    feed_uniform(source);
    source.compress();

    t_digest<float, 32U> copy;
    for (const t_digest<float, 32U>::centroid_type& centroid : source)
    {
        ASSERT_EQ(copy.add(centroid.mean, centroid.weight), status::ok);
    }
    EXPECT_EQ(copy.total(), source.total());
    EXPECT_NEAR(copy.quantile(0.5F), source.quantile(0.5F), 0.002F * 10006.0F);
    EXPECT_NEAR(copy.quantile(0.95F), source.quantile(0.95F), 0.002F * 10006.0F);
}

TEST(TDigestTest, ClearForgetsEverything)
{
    t_digest<float, 16U> digest;
    feed_uniform(digest, 0U, 100U);
    digest.clear();
    EXPECT_TRUE(digest.empty());
    EXPECT_EQ(digest.size(), 0U);
    EXPECT_FLOAT_EQ(digest.quantile(0.5F), 0.0F);
    EXPECT_FLOAT_EQ(digest.min(), 0.0F);
    EXPECT_FLOAT_EQ(digest.max(), 0.0F);

    ASSERT_EQ(digest.add(-3.0F), status::ok);
    EXPECT_FLOAT_EQ(digest.min(), -3.0F);
    EXPECT_FLOAT_EQ(digest.max(), -3.0F);
    EXPECT_FLOAT_EQ(digest.quantile(0.5F), -3.0F);
}

} // namespace
