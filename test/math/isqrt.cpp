
#include <gtest/gtest.h>

#include "castle/math/isqrt.h"

#include <cstdint>

namespace
{

TEST(MathIsqrt, SmallValues)
{
    static_assert(castle::math::isqrt<uint32_t>(0U) == 0U, "isqrt 0");
    static_assert(castle::math::isqrt<uint32_t>(1U) == 1U, "isqrt 1");
    EXPECT_EQ(castle::math::isqrt<uint32_t>(0U), 0U);
    EXPECT_EQ(castle::math::isqrt<uint32_t>(1U), 1U);
}

TEST(MathIsqrt, PerfectAndNonPerfectSquares)
{
    EXPECT_EQ(castle::math::isqrt<uint32_t>(2U), 1U);
    EXPECT_EQ(castle::math::isqrt<uint32_t>(3U), 1U);
    EXPECT_EQ(castle::math::isqrt<uint32_t>(4U), 2U);
    EXPECT_EQ(castle::math::isqrt<uint32_t>(15U), 3U);
    EXPECT_EQ(castle::math::isqrt<uint32_t>(16U), 4U);
    EXPECT_EQ(castle::math::isqrt<uint32_t>(145U), 12U);
    EXPECT_EQ(castle::math::isqrt<uint32_t>(1000000U), 1000U);
    static_assert(castle::math::isqrt<uint32_t>(145U) == 12U, "isqrt");
}

TEST(MathIsqrt, SignedOverload)
{
    EXPECT_EQ(castle::math::isqrt<int32_t>(0), 0);
    EXPECT_EQ(castle::math::isqrt<int32_t>(1), 1);
    EXPECT_EQ(castle::math::isqrt<int32_t>(144), 12);
}

} // namespace
