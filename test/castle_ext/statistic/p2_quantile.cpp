#include <gtest/gtest.h>

#include <limits>
#include <stddef.h>
#include <stdint.h>

#include "castle_ext/statistic/p2_quantile.hpp"

namespace
{
using castle::status;
using estimator = castle::statistic::p2_quantile<double>;

// Deterministic xorshift32 stream of doubles in [0, 1000).
struct uniform_stream
{
    uint32_t state = 2463534242U;

    double next()
    {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;
        return static_cast<double>(state >> 8U) * (1000.0 / 16777216.0);
    }
};

void feed(estimator& e, const double* first, size_t count)
{
    for (size_t i = 0U; i < count; ++i)
    {
        EXPECT_EQ(e.push(first[i]), status::ok);
    }
}

double estimate_of(const estimator& e)
{
    double out = -12345.0;
    EXPECT_EQ(e.quantile(out), status::ok);
    return out;
}

TEST(P2QuantileTest, DefaultIsUnconfigured)
{
    estimator e;
    EXPECT_FALSE(e.configured());
    EXPECT_EQ(e.probability(), 0.0);
    EXPECT_EQ(e.count(), 0U);
    EXPECT_TRUE(e.empty());
    double out = -1.0;
    EXPECT_EQ(e.push(1.0), status::not_configured);
    EXPECT_EQ(e.quantile(out), status::not_configured);
    EXPECT_EQ(out, -1.0);
    EXPECT_EQ(e.count(), 0U);
}

TEST(P2QuantileTest, ConstructorValidAndInvalidProbability)
{
    estimator valid(0.95);
    EXPECT_TRUE(valid.configured());
    EXPECT_DOUBLE_EQ(valid.probability(), 0.95);

    estimator invalid(1.0);
    EXPECT_FALSE(invalid.configured());
    EXPECT_EQ(invalid.probability(), 0.0);
    EXPECT_EQ(invalid.push(1.0), status::not_configured);
}

TEST(P2QuantileTest, ConfigureValidatesProbability)
{
    estimator e;
    EXPECT_EQ(e.configure(0.0), status::invalid_config);
    EXPECT_EQ(e.configure(1.0), status::invalid_config);
    EXPECT_EQ(e.configure(std::numeric_limits<double>::quiet_NaN()), status::invalid_config);
    EXPECT_EQ(e.configure(-1.0), status::invalid_config);
    EXPECT_EQ(e.configure(1.5), status::invalid_config);
    EXPECT_FALSE(e.configured());

    EXPECT_EQ(e.configure(0.5), status::ok);
    EXPECT_TRUE(e.configured());
    EXPECT_DOUBLE_EQ(e.probability(), 0.5);

    // Reconfiguring discards the samples; a failed attempt also unconfigures.
    EXPECT_EQ(e.push(1.0), status::ok);
    EXPECT_EQ(e.configure(0.25), status::ok);
    EXPECT_TRUE(e.empty());
    EXPECT_EQ(e.push(1.0), status::ok);
    EXPECT_EQ(e.configure(0.0), status::invalid_config);
    EXPECT_FALSE(e.configured());
    EXPECT_TRUE(e.empty());
    EXPECT_EQ(e.probability(), 0.0);
}

TEST(P2QuantileTest, EmptyReportsEmpty)
{
    estimator e(0.5);
    double out = -1.0;
    EXPECT_EQ(e.quantile(out), status::empty);
    EXPECT_EQ(out, -1.0);
}

TEST(P2QuantileTest, FewSamplesGiveExactLinearQuantile)
{
    // One sample.
    estimator one(0.5);
    EXPECT_EQ(one.push(7.0), status::ok);
    EXPECT_DOUBLE_EQ(estimate_of(one), 7.0);

    // Three samples inserted ascending, descending, and mixed: median is 2.
    const double orders[3][3] = {{1.0, 2.0, 3.0}, {3.0, 2.0, 1.0}, {2.0, 3.0, 1.0}};
    for (size_t i = 0U; i < 3U; ++i)
    {
        estimator three(0.5);
        feed(three, orders[i], 3U);
        EXPECT_EQ(three.count(), 3U);
        EXPECT_DOUBLE_EQ(estimate_of(three), 2.0);
    }

    // Four samples, p = 0.25: position 0.75 between 1 and 2 -> 1.75.
    const double four_orders[3][4] = {{1.0, 2.0, 3.0, 4.0}, {4.0, 3.0, 2.0, 1.0}, {4.0, 1.0, 3.0, 2.0}};
    for (size_t i = 0U; i < 3U; ++i)
    {
        estimator four(0.25);
        feed(four, four_orders[i], 4U);
        EXPECT_DOUBLE_EQ(estimate_of(four), 1.75);
    }

    // Duplicate values keep the insertion stable.
    estimator dup(0.5);
    const double dups[3] = {2.0, 2.0, 2.0};
    feed(dup, dups, 3U);
    EXPECT_DOUBLE_EQ(estimate_of(dup), 2.0);
}

TEST(P2QuantileTest, FifthSampleInitializesMarkers)
{
    // Through the fifth sample the exact linear quantile is reported:
    // sorted {1,2,3,4,5}, position 0.9 * 4 = 3.6 -> 4 + 0.6 * (5 - 4) = 4.6.
    const double values[5] = {5.0, 1.0, 4.0, 2.0, 3.0};
    estimator e(0.9);
    feed(e, values, 5U);
    EXPECT_EQ(e.count(), 5U);
    EXPECT_DOUBLE_EQ(estimate_of(e), 4.6);

    // The sixth sample switches to the marker estimate (moves marker heights).
    EXPECT_EQ(e.push(6.0), status::ok);
    EXPECT_EQ(e.count(), 6U);
}

TEST(P2QuantileTest, AscendingStreamUsesParabolicUpMoves)
{
    estimator e(0.5);
    for (int i = 1; i <= 100; ++i)
    {
        EXPECT_EQ(e.push(static_cast<double>(i)), status::ok);
    }
    EXPECT_EQ(e.count(), 100U);
    EXPECT_NEAR(estimate_of(e), 50.0, 1e-6);

    estimator e95(0.95);
    for (int i = 1; i <= 100; ++i)
    {
        EXPECT_EQ(e95.push(static_cast<double>(i)), status::ok);
    }
    EXPECT_NEAR(estimate_of(e95), 95.0, 1e-6);
}

TEST(P2QuantileTest, DescendingStreamUsesParabolicDownMoves)
{
    estimator e(0.5);
    for (int i = 100; i >= 1; --i)
    {
        EXPECT_EQ(e.push(static_cast<double>(i)), status::ok);
    }
    EXPECT_NEAR(estimate_of(e), 51.0, 1e-6);

    estimator e95(0.95);
    for (int i = 100; i >= 1; --i)
    {
        EXPECT_EQ(e95.push(static_cast<double>(i)), status::ok);
    }
    EXPECT_NEAR(estimate_of(e95), 96.0, 1e-6);
}

TEST(P2QuantileTest, AlternatingSpikesRejectParabolicPrediction)
{
    double stream[55];
    for (size_t i = 0U; i < 5U; ++i)
    {
        stream[i] = static_cast<double>(i); // 0..4 initialize the markers
    }
    for (size_t i = 0U; i < 50U; ++i)
    {
        stream[5U + i] = ((i % 2U) != 0U) ? 1000.0 : 0.0;
    }

    // p = 0.5: down move whose parabola falls at or below the lower neighbour,
    // then an up move whose parabola exceeds the upper neighbour.
    estimator median(0.5);
    feed(median, stream, 55U);
    EXPECT_NEAR(estimate_of(median), 2.0, 1e-6);

    // p = 0.95: up moves rejected because the parabola reaches the upper neighbour.
    estimator high(0.95);
    feed(high, stream, 55U);
    EXPECT_NEAR(estimate_of(high), 999.9991955, 1e-5);

    // p = 0.1: down moves rejected because the parabola falls to the lower neighbour.
    estimator low(0.1);
    feed(low, stream, 55U);
    EXPECT_NEAR(estimate_of(low), 9.7476e-06, 1e-9);
}

TEST(P2QuantileTest, DuplicateHeavyStreamsStayOrdered)
{
    // Constant stream: every marker collapses to 7.
    estimator constant(0.5);
    for (int i = 0; i < 40; ++i)
    {
        EXPECT_EQ(constant.push(7.0), status::ok);
    }
    EXPECT_DOUBLE_EQ(estimate_of(constant), 7.0);

    // Low quantile of a constant stream: down moves blocked by adjacent positions.
    estimator constant_low(0.1);
    for (int i = 0; i < 40; ++i)
    {
        EXPECT_EQ(constant_low.push(7.0), status::ok);
    }
    EXPECT_DOUBLE_EQ(estimate_of(constant_low), 7.0);

    // Ascending start followed by repeated 3s, p = 0.05.
    estimator dup_low(0.05);
    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_EQ(dup_low.push(static_cast<double>(i)), status::ok);
    }
    for (int i = 0; i < 30; ++i)
    {
        EXPECT_EQ(dup_low.push(3.0), status::ok);
    }
    EXPECT_NEAR(estimate_of(dup_low), 3.0, 1e-6);

    // Descending start followed by repeated 0s, p = 0.5: linear down fallback.
    estimator dup_median(0.5);
    for (int i = 5; i >= 1; --i)
    {
        EXPECT_EQ(dup_median.push(static_cast<double>(i)), status::ok);
    }
    for (int i = 0; i < 30; ++i)
    {
        EXPECT_EQ(dup_median.push(0.0), status::ok);
    }
    const double value = estimate_of(dup_median);
    EXPECT_GE(value, 0.0);
    EXPECT_LT(value, 0.01);
}

TEST(P2QuantileTest, UniformStreamsHitMiddleCells)
{
    uniform_stream stream;
    estimator e(0.5);
    for (int i = 0; i < 10000; ++i)
    {
        EXPECT_EQ(e.push(stream.next()), status::ok);
    }
    EXPECT_EQ(e.count(), 10000U);
    EXPECT_NEAR(estimate_of(e), 500.0, 15.0);
}

TEST(P2QuantileTest, AccuracyOnLargeUniformStream)
{
    uniform_stream stream;
    estimator median(0.5);
    estimator p95(0.95);
    for (int i = 0; i < 100000; ++i)
    {
        const double sample = stream.next();
        EXPECT_EQ(median.push(sample), status::ok);
        EXPECT_EQ(p95.push(sample), status::ok);
    }
    EXPECT_NEAR(estimate_of(median), 500.0, 5.0);
    EXPECT_NEAR(estimate_of(p95), 950.0, 5.0);
}

TEST(P2QuantileTest, RejectsNonFiniteSamples)
{
    estimator e(0.5);
    EXPECT_EQ(e.push(std::numeric_limits<double>::quiet_NaN()), status::invalid_argument);
    EXPECT_EQ(e.push(std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_EQ(e.push(-std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_TRUE(e.empty());

    for (int i = 1; i <= 10; ++i)
    {
        EXPECT_EQ(e.push(static_cast<double>(i)), status::ok);
    }
    EXPECT_EQ(e.push(std::numeric_limits<double>::quiet_NaN()), status::invalid_argument);
    EXPECT_EQ(e.count(), 10U);
}

TEST(P2QuantileTest, ClearKeepsProbability)
{
    estimator e(0.9);
    for (int i = 1; i <= 20; ++i)
    {
        EXPECT_EQ(e.push(static_cast<double>(i)), status::ok);
    }
    e.clear();
    EXPECT_TRUE(e.empty());
    EXPECT_EQ(e.count(), 0U);
    EXPECT_TRUE(e.configured());
    EXPECT_DOUBLE_EQ(e.probability(), 0.9);
    double out = 0.0;
    EXPECT_EQ(e.quantile(out), status::empty);

    // The estimator is fully usable again.
    EXPECT_EQ(e.push(2.0), status::ok);
    EXPECT_DOUBLE_EQ(estimate_of(e), 2.0);
}

TEST(P2QuantileTest, IntegralSamples)
{
    castle::statistic::p2_quantile<int> e(0.5);
    double out = 0.0;
    EXPECT_EQ(e.push(30), status::ok);
    EXPECT_EQ(e.push(10), status::ok);
    EXPECT_EQ(e.push(20), status::ok);
    EXPECT_EQ(e.quantile(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 20.0);

    castle::statistic::p2_quantile<int> ramp(0.5);
    for (int i = 1; i <= 10; ++i)
    {
        EXPECT_EQ(ramp.push(i), status::ok);
    }
    EXPECT_EQ(ramp.quantile(out), status::ok);
    EXPECT_NEAR(out, 5.0, 1e-6);
}

TEST(P2QuantileTest, FloatSamples)
{
    castle::statistic::p2_quantile<float> e(0.5f);
    float out = 0.0f;
    for (int i = 1; i <= 100; ++i)
    {
        EXPECT_EQ(e.push(static_cast<float>(i)), status::ok);
    }
    EXPECT_EQ(e.quantile(out), status::ok);
    EXPECT_NEAR(out, 50.0f, 0.5f);
}
} // namespace
