
#include <gtest/gtest.h>

#include "castle/algorithm/algorithm.hpp"

namespace
{

TEST(MathMinMax, PairwiseOrders)
{
    EXPECT_EQ(castle::min(2, 5), 2);
    EXPECT_EQ(castle::min(5, 2), 2);
    EXPECT_EQ(castle::min(3, 3), 3);

    EXPECT_EQ(castle::max(2, 5), 5);
    EXPECT_EQ(castle::max(5, 2), 5);
    EXPECT_EQ(castle::max(3, 3), 3);
}

TEST(MathMinMax, ThreeValues)
{
    EXPECT_EQ(castle::min3(5, 2, 4), 2);
    EXPECT_EQ(castle::min3(2, 5, 4), 2);
    EXPECT_EQ(castle::min3(4, 2, 5), 2);
    EXPECT_EQ(castle::max3(5, 2, 4), 5);
    EXPECT_EQ(castle::max3(2, 5, 4), 5);
    EXPECT_EQ(castle::max3(4, 2, 5), 5);

    static_assert(castle::min3(5, 2, 4) == 2, "min3");
    static_assert(castle::max3(5, 2, 4) == 5, "max3");
}

} // namespace
