
#include <gtest/gtest.h>

#include "castle/math/geometry/circle3d.h"

namespace
{

TEST(GeometryCircle3d, DefaultAndConstruction)
{
    const castle::math::circle3d<double> empty;
    EXPECT_EQ(empty.center(), castle::math::point3d<double>(0.0, 0.0, 0.0));
    EXPECT_EQ(empty.normal(), castle::math::vector3d<double>(0.0, 0.0, 1.0));
    EXPECT_DOUBLE_EQ(empty.radius(), 0.0);

    const castle::math::circle3d<double> circle(
        castle::math::point3d<double>(1.0, 2.0, 3.0),
        castle::math::vector3d<double>(0.0, 0.0, 2.0),
        4.0);

    EXPECT_EQ(circle.center(), castle::math::point3d<double>(1.0, 2.0, 3.0));
    EXPECT_EQ(circle.normal(), castle::math::vector3d<double>(0.0, 0.0, 2.0));
    EXPECT_DOUBLE_EQ(circle.radius(), 4.0);
    EXPECT_EQ(circle.plane().point(), circle.center());
    EXPECT_EQ(circle.plane().normal(), circle.normal());
}

TEST(GeometryCircle3d, Contains)
{
    const castle::math::circle3d<double> circle(
        castle::math::point3d<double>(0.0, 0.0, 0.0),
        castle::math::vector3d<double>(0.0, 0.0, 1.0),
        2.0);

    EXPECT_TRUE(circle.contains(castle::math::point3d<double>(0.0, 0.0, 0.0)));
    EXPECT_TRUE(circle.contains(castle::math::point3d<double>(2.0, 0.0, 0.0)));
    EXPECT_FALSE(circle.contains(castle::math::point3d<double>(3.0, 0.0, 0.0)));
    EXPECT_FALSE(circle.contains(castle::math::point3d<double>(0.0, 0.0, 1.0)));
    EXPECT_TRUE(circle.contains(castle::math::point3d<double>(0.0, 0.0, 1e-4), 1e-3));

    const castle::math::circle3d<float> floating_circle(
        castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
        castle::math::vector3d<float>(0.0f, 0.0f, 1.0f),
        2.0f);
    const castle::math::point3d<float> floating_off_plane(0.0f, 0.0f, 1.0f);
    EXPECT_FALSE(floating_circle.contains(floating_off_plane));
}

TEST(GeometryCircle3d, OnCircle)
{
    const castle::math::circle3d<double> circle(
        castle::math::point3d<double>(0.0, 0.0, 0.0),
        castle::math::vector3d<double>(0.0, 0.0, 1.0),
        2.0);

    EXPECT_TRUE(circle.on_circle(castle::math::point3d<double>(2.0, 0.0, 0.0)));
    EXPECT_FALSE(circle.on_circle(castle::math::point3d<double>(1.0, 0.0, 0.0)));
    EXPECT_FALSE(circle.on_circle(castle::math::point3d<double>(0.0, 0.0, 1.0)));
    EXPECT_TRUE(circle.on_circle(castle::math::point3d<double>(2.0, 0.0, 1e-4), 1e-3));

    const castle::math::circle3d<float> floating_circle(
        castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
        castle::math::vector3d<float>(0.0f, 0.0f, 1.0f),
        2.0f);
    const castle::math::point3d<float> floating_on(2.0f, 0.0f, 0.0f);
    EXPECT_TRUE(floating_circle.on_circle(floating_on));
    EXPECT_FALSE(floating_circle.on_circle(
        castle::math::point3d<float>(0.0f, 0.0f, 0.0f)));
}

TEST(GeometryCircle3d, AxialRoundoffGuard)
{
    const castle::math::circle3d<float> circle(
        castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
        castle::math::vector3d<float>(1.0f, 1.0f, 1.0f),
        1.0f);

    const castle::math::point3d<float> point(
        1000000000.0f, 1000000000.0f, 1000000000.0f);

    // The projection round-off makes the computed in-plane squared distance
    // slightly negative on float32. A sufficiently large plane tolerance
    // reaches the defensive axial < 0 branch.
    EXPECT_FALSE(circle.contains(point, 2000000000.0f));
    EXPECT_FALSE(circle.on_circle(point, 2000000000.0f));
}

} // namespace
