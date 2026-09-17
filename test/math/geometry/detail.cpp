
#include <gtest/gtest.h>

#include "castle/math/geometry/detail.h"

namespace
{

TEST(GeometryDetail, TwoDimensionalProducts)
{
    EXPECT_EQ(castle::math::detail::cross2(1, 2, 3, 4), -2);
    EXPECT_EQ(castle::math::detail::dot2(1, 2, 3, 4), 11);
    EXPECT_EQ(castle::math::detail::cross2(0, 0, 0, 0), 0);
}

TEST(GeometryDetail, ThreeDimensionalProducts)
{
    EXPECT_EQ(castle::math::detail::dot3(1, 2, 3, 2, 1, 0), 4);
    EXPECT_EQ(castle::math::detail::cross3_x(1, 0, 0, 1), 1);
    EXPECT_EQ(castle::math::detail::cross3_y(1, 0, 0, 1), 1);
    EXPECT_EQ(castle::math::detail::cross3_z(1, 0, 0, 1), 1);
    EXPECT_EQ(castle::math::detail::cross2(2, 0, 0, 2), 4);
}

TEST(GeometryDetail, NearZero)
{
    EXPECT_TRUE(castle::math::detail::near_zero(0.0, 0.1));
    EXPECT_TRUE(castle::math::detail::near_zero(0.05, 0.1));
    EXPECT_FALSE(castle::math::detail::near_zero(0.2, 0.1));
}

} // namespace
