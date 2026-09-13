
#include <gtest/gtest.h>

#include "castle/math/geometry/circle2d.h"

namespace
{

TEST(GeometryCircle2d, DefaultAndConstruction)
{
    const castle::math::circle2d<double> empty;
    EXPECT_EQ(empty.center(), castle::math::point2d<double>(0.0, 0.0));
    EXPECT_DOUBLE_EQ(empty.radius(), 0.0);

    const castle::math::circle2d<double> circle(
        castle::math::point2d<double>(1.0, -2.0), 3.0);
    EXPECT_EQ(circle.center(), castle::math::point2d<double>(1.0, -2.0));
    EXPECT_DOUBLE_EQ(circle.radius(), 3.0);
}

TEST(GeometryCircle2d, Contains)
{
    const castle::math::circle2d<double> circle(
        castle::math::point2d<double>(0.0, 0.0), 2.0);

    EXPECT_TRUE(circle.contains(castle::math::point2d<double>(0.0, 0.0)));
    EXPECT_TRUE(circle.contains(castle::math::point2d<double>(2.0, 0.0)));
    EXPECT_FALSE(circle.contains(castle::math::point2d<double>(3.0, 0.0)));
    EXPECT_TRUE(circle.contains(castle::math::point2d<double>(2.05, 0.0), 0.1));
}

TEST(GeometryCircle2d, OnCircleAndToleranceRadiusBranch)
{
    const castle::math::circle2d<double> circle(
        castle::math::point2d<double>(0.0, 0.0), 2.0);

    EXPECT_TRUE(circle.on_circle(castle::math::point2d<double>(2.0, 0.0)));
    EXPECT_FALSE(circle.on_circle(castle::math::point2d<double>(1.0, 0.0)));
    EXPECT_TRUE(circle.on_circle(castle::math::point2d<double>(2.05, 0.0), 0.1));

    const castle::math::circle2d<double> zero_radius(
        castle::math::point2d<double>(0.0, 0.0), 0.0);
    EXPECT_TRUE(zero_radius.on_circle(castle::math::point2d<double>(0.0, 0.0), 0.1));
    EXPECT_FALSE(zero_radius.on_circle(castle::math::point2d<double>(0.2, 0.0), 0.1));
}

} // namespace
