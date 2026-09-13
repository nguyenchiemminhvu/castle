
#include <gtest/gtest.h>

#include "castle/math/ceil_div.h"

#include <cstdint>

namespace
{

TEST(MathCeilDiv, PositiveAndExact)
{
    EXPECT_EQ(castle::math::ceil_div(0, 5), 0);
    EXPECT_EQ(castle::math::ceil_div(1, 5), 1);
    EXPECT_EQ(castle::math::ceil_div(10, 5), 2);
    EXPECT_EQ(castle::math::ceil_div(11, 5), 3);
    EXPECT_EQ(castle::math::ceil_div(20, 7), 3);

    static_assert(castle::math::ceil_div(11, 5) == 3, "ceil_div");
}

TEST(MathCeilDiv, NegativeNumerator)
{
    EXPECT_EQ(castle::math::ceil_div(-1, 5), 0);
    EXPECT_EQ(castle::math::ceil_div(-5, 5), -1);
    EXPECT_EQ(castle::math::ceil_div(-11, 5), -2);

    static_assert(castle::math::ceil_div(-11, 5) == -2, "ceil_div negative");
}

TEST(MathCeilDiv, Unsigned)
{
    EXPECT_EQ(castle::math::ceil_div<uint32_t>(100U, 10U), 10U);
    EXPECT_EQ(castle::math::ceil_div<uint32_t>(101U, 10U), 11U);
}

} // namespace
