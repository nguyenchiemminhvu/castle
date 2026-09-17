#include <gtest/gtest.h>

#include "castle/math/sqrt.h"

namespace
{

// ---------------------------------------------------------------------------
// castle::sqrt<Value>::value - compile-time integer square root (floor).
// Note: this lives in namespace castle, not castle::math.
// ---------------------------------------------------------------------------
TEST(MathSqrt, PerfectSquares)
{
    static_assert(castle::sqrt<0U>::value == 0u, "");
    static_assert(castle::sqrt<1U>::value == 1u, "");
    static_assert(castle::sqrt<4U>::value == 2u, "");
    static_assert(castle::sqrt<9U>::value == 3u, "");
    static_assert(castle::sqrt<16U>::value == 4u, "");
    static_assert(castle::sqrt<144U>::value == 12u, "");
    static_assert(castle::sqrt<10000U>::value == 100u, "");
    SUCCEED();
}

TEST(MathSqrt, NonPerfectSquaresFloor)
{
    static_assert(castle::sqrt<2U>::value == 1u, "");
    static_assert(castle::sqrt<3U>::value == 1u, "");
    static_assert(castle::sqrt<8U>::value == 2u, "");
    static_assert(castle::sqrt<15U>::value == 3u, "");
    static_assert(castle::sqrt<143U>::value == 11u, "");
    static_assert(castle::sqrt<145U>::value == 12u, "");
    static_assert(castle::sqrt<9999U>::value == 99u, "");
    SUCCEED();
}

TEST(MathSqrt, Boundaries)
{
    // Values straddling a perfect square boundary.
    static_assert(castle::sqrt<24U>::value == 4u, "");
    static_assert(castle::sqrt<25U>::value == 5u, "");
    static_assert(castle::sqrt<26U>::value == 5u, "");
    SUCCEED();
}

TEST(MathSqrt, LargeValue)
{
    static_assert(castle::sqrt<1000000U>::value == 1000u, "");
    static_assert(castle::sqrt<4294967296U>::value == 65536u, "");
    SUCCEED();
}

TEST(MathSqrt, TypeAndValueType)
{
    static_assert(castle::sqrt<144U>::type::value == 12u, "");
    const castle::sqrt<49U>::value_type v = castle::sqrt<49U>::value;
    EXPECT_EQ(v, 7u);
}

TEST(MathSqrt, RuntimeReadable)
{
    EXPECT_EQ(castle::sqrt<144U>::value, 12u);
    EXPECT_EQ(castle::sqrt<145U>::value, 12u);
    EXPECT_EQ(castle::sqrt<0U>::value, 0u);
}

} // namespace
