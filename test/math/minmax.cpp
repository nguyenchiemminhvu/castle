
#include <gtest/gtest.h>

#include "castle/math/minmax.h"

namespace
{

TEST(MathMinMax, PairwiseOrders)
{
    EXPECT_EQ(castle::math::min(2, 5), 2);
    EXPECT_EQ(castle::math::min(5, 2), 2);
    EXPECT_EQ(castle::math::min(3, 3), 3);

    EXPECT_EQ(castle::math::max(2, 5), 5);
    EXPECT_EQ(castle::math::max(5, 2), 5);
    EXPECT_EQ(castle::math::max(3, 3), 3);
}

TEST(MathMinMax, ThreeValues)
{
    EXPECT_EQ(castle::math::min3(5, 2, 4), 2);
    EXPECT_EQ(castle::math::min3(2, 5, 4), 2);
    EXPECT_EQ(castle::math::min3(4, 2, 5), 2);
    EXPECT_EQ(castle::math::max3(5, 2, 4), 5);
    EXPECT_EQ(castle::math::max3(2, 5, 4), 5);
    EXPECT_EQ(castle::math::max3(4, 2, 5), 5);

    static_assert(castle::math::min3(5, 2, 4) == 2, "min3");
    static_assert(castle::math::max3(5, 2, 4) == 5, "max3");
}

} // namespace
