
#include <gtest/gtest.h>

#include "castle/math/sign.h"

#include <cstdint>

namespace
{

TEST(MathSign, NegativeZeroPositive)
{
    EXPECT_EQ(castle::math::sign(-5), -1);
    EXPECT_EQ(castle::math::sign(0), 0);
    EXPECT_EQ(castle::math::sign(5), 1);
    EXPECT_EQ(castle::math::sign(-0.5), -1);
    EXPECT_EQ(castle::math::sign(0.0), 0);
    EXPECT_EQ(castle::math::sign(0.5), 1);
}

TEST(MathSign, Unsigned)
{
    EXPECT_EQ(castle::math::sign(uint32_t{0}), 0);
    EXPECT_EQ(castle::math::sign(uint32_t{7}), 1);
    static_assert(castle::math::sign(-5) == -1, "sign");
    static_assert(castle::math::sign(5) == 1, "sign");
}

} // namespace
