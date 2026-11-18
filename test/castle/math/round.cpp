#include <gtest/gtest.h>

#include "castle/math/round.hpp"

#include <stdint.h>

namespace
{

using castle::math::round_half_away;

TEST(MathRound, RoundsToNearestInteger)
{
    EXPECT_EQ(round_half_away<int32_t>(0.0), 0);
    EXPECT_EQ(round_half_away<int32_t>(0.49), 0);
    EXPECT_EQ(round_half_away<int32_t>(0.51), 1);
    EXPECT_EQ(round_half_away<int32_t>(-0.49), 0);
    EXPECT_EQ(round_half_away<int32_t>(-0.51), -1);
    EXPECT_EQ(round_half_away<int32_t>(1234.4), 1234);
    EXPECT_EQ(round_half_away<int32_t>(-1234.6), -1235);
}

TEST(MathRound, TiesRoundAwayFromZero)
{
    EXPECT_EQ(round_half_away<int32_t>(0.5), 1);
    EXPECT_EQ(round_half_away<int32_t>(-0.5), -1);
    EXPECT_EQ(round_half_away<int32_t>(2.5), 3);
    EXPECT_EQ(round_half_away<int32_t>(-2.5), -3);
    EXPECT_EQ(round_half_away<uint32_t>(2.5), 3U);
    EXPECT_EQ(round_half_away<int16_t>(-8191.5), -8192);
}

TEST(MathRound, AcceptsFloatAndDoubleSources)
{
    EXPECT_EQ(round_half_away<int32_t>(2.5f), 3);
    EXPECT_EQ(round_half_away<int32_t>(-2.5f), -3);
    EXPECT_EQ(round_half_away<uint8_t>(254.6f), 255U);
}

TEST(MathRound, HonoursTheDestinationWidth)
{
    // A 38-bit RTCM coordinate needs a 64-bit destination.
    EXPECT_EQ(round_half_away<int64_t>(137438953471.4), 137438953471LL);
    EXPECT_EQ(round_half_away<int64_t>(-137438953472.5), -137438953473LL);
    EXPECT_EQ(round_half_away<uint32_t>(4294967294.6), 4294967295U);
}

TEST(MathRound, IsAConstantExpression)
{
    static_assert(round_half_away<int32_t>(2.5) == 3, "usable in constant expressions");
    static_assert(round_half_away<int32_t>(-2.5) == -3, "usable in constant expressions");
    SUCCEED();
}

} // namespace
