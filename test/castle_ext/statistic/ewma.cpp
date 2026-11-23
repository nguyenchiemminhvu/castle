#include <gtest/gtest.h>

#include <limits>

#include "castle_ext/statistic/ewma.hpp"

namespace
{
using castle::status;

TEST(EwmaTest, UnconfiguredRejectsPush)
{
    castle::statistic::ewma<double> filter;
    EXPECT_FALSE(filter.configured());
    EXPECT_EQ(filter.alpha(), 0.0);
    EXPECT_TRUE(filter.empty());
    EXPECT_EQ(filter.push(1.0), status::not_configured);
    EXPECT_TRUE(filter.empty());
}

TEST(EwmaTest, ConstructorValidAndInvalidAlpha)
{
    castle::statistic::ewma<double> valid(0.25);
    EXPECT_TRUE(valid.configured());
    EXPECT_DOUBLE_EQ(valid.alpha(), 0.25);

    castle::statistic::ewma<double> zero(0.0);
    EXPECT_FALSE(zero.configured());
    EXPECT_EQ(zero.push(1.0), status::not_configured);
}

TEST(EwmaTest, ConfigureRejectsInvalidAlpha)
{
    castle::statistic::ewma<double> filter;
    EXPECT_EQ(filter.configure(0.0), status::invalid_config);
    EXPECT_FALSE(filter.configured());
    EXPECT_EQ(filter.configure(-0.1), status::invalid_config);
    EXPECT_EQ(filter.configure(1.5), status::invalid_config);
    EXPECT_EQ(filter.configure(std::numeric_limits<double>::quiet_NaN()), status::invalid_config);
    EXPECT_FALSE(filter.configured());
    EXPECT_EQ(filter.alpha(), 0.0);

    EXPECT_EQ(filter.configure(0.5), status::ok);
    EXPECT_TRUE(filter.configured());
    EXPECT_EQ(filter.configure(1.0), status::ok);
    EXPECT_DOUBLE_EQ(filter.alpha(), 1.0);

    // A failed configure drops a previously valid setting.
    EXPECT_EQ(filter.configure(2.0), status::invalid_config);
    EXPECT_FALSE(filter.configured());
}

TEST(EwmaTest, EmptyQueriesReportEmpty)
{
    castle::statistic::ewma<double> filter(0.5);
    double out = -1.0;
    EXPECT_EQ(filter.mean(out), status::empty);
    EXPECT_EQ(filter.variance(out), status::empty);
    EXPECT_EQ(filter.stddev(out), status::empty);
    EXPECT_EQ(out, -1.0);
}

TEST(EwmaTest, UpdatesMeanAndVariance)
{
    castle::statistic::ewma<double> filter(0.5);
    double out = 0.0;

    // First sample: mean 10, variance 0.
    EXPECT_EQ(filter.push(10.0), status::ok);
    EXPECT_FALSE(filter.empty());
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 10.0);
    EXPECT_EQ(filter.variance(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(filter.stddev(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);

    // diff 10, increment 5 -> mean 15, variance 0.5 * (0 + 10 * 5) = 25.
    EXPECT_EQ(filter.push(20.0), status::ok);
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 15.0);
    EXPECT_EQ(filter.variance(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 25.0);
    EXPECT_EQ(filter.stddev(out), status::ok);
    EXPECT_NEAR(out, 5.0, 1e-9);

    // diff 0 -> mean 15, variance 0.5 * 25 = 12.5.
    EXPECT_EQ(filter.push(15.0), status::ok);
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 15.0);
    EXPECT_EQ(filter.variance(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 12.5);
    EXPECT_EQ(filter.stddev(out), status::ok);
    EXPECT_NEAR(out, 3.5355339059327378, 1e-9);
}

TEST(EwmaTest, AlphaOneTracksInput)
{
    castle::statistic::ewma<double> filter(1.0);
    double out = 0.0;
    EXPECT_EQ(filter.push(3.0), status::ok);
    EXPECT_EQ(filter.push(7.0), status::ok);
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 7.0);
    EXPECT_EQ(filter.variance(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
}

TEST(EwmaTest, RejectsNonFiniteSamples)
{
    castle::statistic::ewma<double> filter(0.5);
    EXPECT_EQ(filter.push(std::numeric_limits<double>::quiet_NaN()), status::invalid_argument);
    EXPECT_EQ(filter.push(std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_EQ(filter.push(-std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_TRUE(filter.empty());

    EXPECT_EQ(filter.push(4.0), status::ok);
    EXPECT_EQ(filter.push(std::numeric_limits<double>::quiet_NaN()), status::invalid_argument);
    double out = 0.0;
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 4.0);
}

TEST(EwmaTest, ClearKeepsAlpha)
{
    castle::statistic::ewma<double> filter(0.5);
    EXPECT_EQ(filter.push(10.0), status::ok);
    EXPECT_EQ(filter.push(20.0), status::ok);
    filter.clear();
    EXPECT_TRUE(filter.empty());
    EXPECT_TRUE(filter.configured());
    EXPECT_DOUBLE_EQ(filter.alpha(), 0.5);
    double out = 0.0;
    EXPECT_EQ(filter.mean(out), status::empty);
    EXPECT_EQ(filter.push(8.0), status::ok);
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 8.0);
}

TEST(EwmaTest, IntegralSamples)
{
    castle::statistic::ewma<int> filter(0.5);
    double out = 0.0;
    EXPECT_EQ(filter.push(10), status::ok);
    EXPECT_EQ(filter.push(20), status::ok);
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 15.0);
    EXPECT_EQ(filter.variance(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 25.0);
}

TEST(EwmaTest, FloatSamples)
{
    castle::statistic::ewma<float> filter(0.2f);
    float out = 0.0f;
    EXPECT_EQ(filter.push(10.0f), status::ok);
    EXPECT_EQ(filter.push(12.0f), status::ok);
    EXPECT_EQ(filter.mean(out), status::ok);
    EXPECT_NEAR(out, 10.4f, 1e-5f);
    EXPECT_EQ(filter.variance(out), status::ok);
    EXPECT_NEAR(out, 0.64f, 1e-5f);
}
} // namespace
