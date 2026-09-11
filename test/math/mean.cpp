#include <gtest/gtest.h>

#include "castle/math/mean.h"

#include <stdint.h>
#include <vector>

namespace
{

// ---------------------------------------------------------------------------
// mean<TInput> - running accumulator
// ---------------------------------------------------------------------------
TEST(MathMean, EmptyIsZero)
{
    castle::math::mean<int> m;
    EXPECT_EQ(m.count(), 0u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 0.0);
}

TEST(MathMean, SingleValue)
{
    castle::math::mean<int> m;
    m.add(42);
    EXPECT_EQ(m.count(), 1u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 42.0);
}

TEST(MathMean, MultipleValues)
{
    castle::math::mean<int> m;
    m.add(1);
    m.add(2);
    m.add(3);
    m.add(4);
    EXPECT_EQ(m.count(), 4u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 2.5);
}

TEST(MathMean, NegativeValues)
{
    castle::math::mean<int> m;
    m.add(-2);
    m.add(2);
    m.add(-4);
    m.add(4);
    EXPECT_EQ(m.count(), 4u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 0.0);
}

TEST(MathMean, FunctionalSyntax)
{
    castle::math::mean<int> m;
    m(10);
    m(20);
    m(30);
    EXPECT_EQ(m.count(), 3u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 20.0);
}

TEST(MathMean, ImplicitDoubleConversion)
{
    castle::math::mean<int> m;
    m.add(2);
    m.add(4);
    const double avg = m;
    EXPECT_DOUBLE_EQ(avg, 3.0);
}

TEST(MathMean, IteratorRangeConstructor)
{
    const std::vector<int> data{2, 4, 6, 8, 10};
    castle::math::mean<int> m(data.begin(), data.end());
    EXPECT_EQ(m.count(), 5u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 6.0);
}

TEST(MathMean, AddRange)
{
    const std::vector<int> data{1, 2, 3};
    castle::math::mean<int> m;
    m.add(data.begin(), data.end());
    EXPECT_EQ(m.count(), 3u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 2.0);
}

TEST(MathMean, Clear)
{
    castle::math::mean<int> m;
    m.add(100);
    m.add(200);
    m.clear();
    EXPECT_EQ(m.count(), 0u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 0.0);

    m.add(5);
    EXPECT_EQ(m.count(), 1u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 5.0);
}

TEST(MathMean, WideAccumulatorAvoidsOverflow)
{
    // uint8_t inputs, uint32_t accumulator: sum exceeds 255 without wrapping.
    castle::math::mean<uint8_t, uint32_t> m;
    for (int i = 0; i < 10; ++i)
    {
        m.add(static_cast<uint8_t>(200));
    }
    EXPECT_EQ(m.count(), 10u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 200.0);
}

TEST(MathMean, FloatingInput)
{
    castle::math::mean<double> m;
    m.add(1.5);
    m.add(2.5);
    m.add(4.0);
    EXPECT_EQ(m.count(), 3u);
    EXPECT_DOUBLE_EQ(m.get_mean(), 8.0 / 3.0);
}

TEST(MathMean, RecalculatesAfterMoreData)
{
    castle::math::mean<int> m;
    m.add(10);
    EXPECT_DOUBLE_EQ(m.get_mean(), 10.0);
    m.add(30);
    EXPECT_DOUBLE_EQ(m.get_mean(), 20.0);
}

} // namespace
