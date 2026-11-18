
#include <gtest/gtest.h>

#include "castle/math/interpolation/lerp.hpp"

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

TEST(MathLerp, LinearInterpolateSampledCurve)
{
    const double xs[] = {0.0, 1.0, 2.0};
    const double ys[] = {0.0, 2.0, 8.0};
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 3U, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 3U, 0.5), 1.0);
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 3U, 1.5), 5.0);
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 3U, 2.0), 8.0);
}

TEST(MathLerp, LinearInterpolateClampsOutsideRange)
{
    const double xs[] = {0.0, 1.0, 2.0};
    const double ys[] = {0.0, 2.0, 8.0};
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 3U, -1.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 3U, 5.0), 8.0);
}

TEST(MathLerp, LinearInterpolateInvalidInput)
{
    const double xs[] = {0.0, 1.0};
    const double ys[] = {3.0, 4.0};
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate<double>(nullptr, ys, 2U, 0.5), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 0U, 0.5), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::linear_interpolate(xs, ys, 1U, 5.0), 3.0);
}

} // namespace
