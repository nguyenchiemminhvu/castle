
#include <gtest/gtest.h>

#include "castle/math/floor_div.h"

#include <cstdint>

namespace
{

TEST(MathFloorDiv, PositiveAndExact)
{
    EXPECT_EQ(castle::math::floor_div(0, 5), 0);
    EXPECT_EQ(castle::math::floor_div(1, 5), 0);
    EXPECT_EQ(castle::math::floor_div(10, 5), 2);
    EXPECT_EQ(castle::math::floor_div(11, 5), 2);
    EXPECT_EQ(castle::math::floor_div(20, 7), 2);

    static_assert(castle::math::floor_div(11, 5) == 2, "floor_div");
}

TEST(MathFloorDiv, NegativeNumerator)
{
    EXPECT_EQ(castle::math::floor_div(-1, 5), -1);
    EXPECT_EQ(castle::math::floor_div(-5, 5), -1);
    EXPECT_EQ(castle::math::floor_div(-11, 5), -3);

    static_assert(castle::math::floor_div(-11, 5) == -3, "floor_div negative");
}

TEST(MathFloorDiv, Unsigned)
{
    EXPECT_EQ(castle::math::floor_div<uint32_t>(100U, 10U), 10U);
    EXPECT_EQ(castle::math::floor_div<uint32_t>(101U, 10U), 10U);
}

} // namespace
