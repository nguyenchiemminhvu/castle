#include <gtest/gtest.h>

#include "castle/math/math.h"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// math.h is an umbrella header: the aggregate API must be reachable.
// ---------------------------------------------------------------------------
TEST(MathUmbrella, AggregateApiExposed)
{
    // fib / abs / clamp / sqrt / gcd / lcm / logarithm / ratio all visible.
    EXPECT_EQ(castle::math::fib(10u), 55u);
    EXPECT_EQ(castle::math::abs<int>(-4), 4);
    EXPECT_EQ(castle::math::clamp(12, 0, 10), 10);
    EXPECT_EQ(castle::sqrt<144U>::value, 12u);

    static_assert(castle::math::gcd<48, 36>::value == 12, "");
    static_assert(castle::math::lcm<4, 6>::value == 12, "");
    static_assert(castle::math::logarithm<8, 2>::value == 3u, "");
    static_assert(castle::math::ratio<2, 4>::num == 1, "");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// is_equal - floating point equality within epsilon
// ---------------------------------------------------------------------------
TEST(MathIsEqual, ExactlyEqual)
{
    EXPECT_TRUE(castle::math::is_equal(1.0, 1.0));
    EXPECT_TRUE(castle::math::is_equal(0.0, 0.0));
    EXPECT_TRUE(castle::math::is_equal(-2.5, -2.5));
    EXPECT_TRUE(castle::math::is_equal(3.5f, 3.5f));
}

TEST(MathIsEqual, WithinEpsilon)
{
    // Differences far smaller than epsilon compare equal.
    EXPECT_TRUE(castle::math::is_equal(1.0, 1.0 + 1e-18));
    EXPECT_TRUE(castle::math::is_equal(1.0f, 1.0f + 1e-9f));
}

TEST(MathIsEqual, NotEqual)
{
    EXPECT_FALSE(castle::math::is_equal(1.0, 2.0));
    EXPECT_FALSE(castle::math::is_equal(0.0, 1.0));
    EXPECT_FALSE(castle::math::is_equal(1.0f, 1.5f));
}

// ---------------------------------------------------------------------------
// is_zero - magnitude within epsilon of zero
// ---------------------------------------------------------------------------
TEST(MathIsZero, ZeroAndTiny)
{
    EXPECT_TRUE(castle::math::is_zero(0.0));
    EXPECT_TRUE(castle::math::is_zero(-0.0));
    EXPECT_TRUE(castle::math::is_zero(1e-18));
    EXPECT_TRUE(castle::math::is_zero(-1e-18));
}

TEST(MathIsZero, NonZero)
{
    EXPECT_FALSE(castle::math::is_zero(1.0));
    EXPECT_FALSE(castle::math::is_zero(-0.5));
    EXPECT_FALSE(castle::math::is_zero(0.001f));
}

} // namespace
