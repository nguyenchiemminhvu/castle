
#include <gtest/gtest.h>

#include "castle/math/lerp.h"

namespace
{

TEST(MathLerp, EndPointsAndMidPoint)
{
    EXPECT_DOUBLE_EQ(castle::math::lerp(0.0, 10.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::lerp(0.0, 10.0, 1.0), 10.0);
    EXPECT_DOUBLE_EQ(castle::math::lerp(0.0, 10.0, 0.5), 5.0);
    static_assert(castle::math::lerp(0.0, 10.0, 0.25) == 2.5, "lerp");
}

TEST(MathLerp, Extrapolation)
{
    EXPECT_DOUBLE_EQ(castle::math::lerp(10.0, 20.0, -1.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::lerp(10.0, 20.0, 2.0), 30.0);
}

} // namespace
