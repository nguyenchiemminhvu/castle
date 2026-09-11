#include <gtest/gtest.h>

#include "castle/math/clamp.h"

namespace
{

// ---------------------------------------------------------------------------
// clamp - runtime behaviour
// ---------------------------------------------------------------------------
TEST(MathClamp, WithinRange)
{
    EXPECT_EQ(castle::math::clamp(5, 0, 10), 5);
    EXPECT_EQ(castle::math::clamp(0, 0, 10), 0);
    EXPECT_EQ(castle::math::clamp(10, 0, 10), 10);
}

TEST(MathClamp, BelowLow)
{
    EXPECT_EQ(castle::math::clamp(-5, 0, 10), 0);
    EXPECT_EQ(castle::math::clamp(-1, 0, 10), 0);
}

TEST(MathClamp, AboveHigh)
{
    EXPECT_EQ(castle::math::clamp(15, 0, 10), 10);
    EXPECT_EQ(castle::math::clamp(11, 0, 10), 10);
}

TEST(MathClamp, NegativeRange)
{
    EXPECT_EQ(castle::math::clamp(-5, -10, -1), -5);
    EXPECT_EQ(castle::math::clamp(-20, -10, -1), -10);
    EXPECT_EQ(castle::math::clamp(5, -10, -1), -1);
}

TEST(MathClamp, DegenerateRange)
{
    // low == high: everything collapses to that single value.
    EXPECT_EQ(castle::math::clamp(3, 7, 7), 7);
    EXPECT_EQ(castle::math::clamp(7, 7, 7), 7);
    EXPECT_EQ(castle::math::clamp(9, 7, 7), 7);
}

TEST(MathClamp, Unsigned)
{
    EXPECT_EQ(castle::math::clamp<unsigned>(200u, 0u, 100u), 100u);
    EXPECT_EQ(castle::math::clamp<unsigned>(50u, 0u, 100u), 50u);
}

TEST(MathClamp, FloatingPoint)
{
    EXPECT_DOUBLE_EQ(castle::math::clamp(1.5, 0.0, 1.0), 1.0);
    EXPECT_DOUBLE_EQ(castle::math::clamp(-0.5, 0.0, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::clamp(0.5, 0.0, 1.0), 0.5);
}

// ---------------------------------------------------------------------------
// clamp - constexpr evaluation
// ---------------------------------------------------------------------------
TEST(MathClamp, Constexpr)
{
    static_assert(castle::math::clamp(5, 0, 10) == 5, "clamp within range");
    static_assert(castle::math::clamp(-5, 0, 10) == 0, "clamp below low");
    static_assert(castle::math::clamp(15, 0, 10) == 10, "clamp above high");
    SUCCEED();
}

} // namespace
