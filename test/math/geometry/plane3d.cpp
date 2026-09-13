
#include <gtest/gtest.h>

#include "castle/math/geometry/plane3d.h"

namespace
{

TEST(GeometryPlane3d, ConstructionAndAccessors)
{
    const castle::math::plane3d<double> plane(
        castle::math::point3d<double>(0.0, 0.0, 2.0),
        castle::math::vector3d<double>(0.0, 0.0, 1.0));

    EXPECT_EQ(plane.point(), castle::math::point3d<double>(0.0, 0.0, 2.0));
    EXPECT_EQ(plane.normal(), castle::math::vector3d<double>(0.0, 0.0, 1.0));
}

TEST(GeometryPlane3d, DefaultAndSignedValue)
{
    const castle::math::plane3d<int> empty;
    EXPECT_TRUE(empty.normal().degenerate());
    EXPECT_EQ(empty.signed_value(castle::math::point3d<int>(1, 2, 3)), 0);

    const castle::math::plane3d<int> plane(
        castle::math::point3d<int>(0, 0, 2),
        castle::math::vector3d<int>(0, 0, 1));

    EXPECT_EQ(plane.signed_value(castle::math::point3d<int>(0, 0, 5)), 3);
    EXPECT_EQ(plane.signed_value(castle::math::point3d<int>(0, 0, -1)), -3);
}

TEST(GeometryPlane3d, ContainsWithAndWithoutTolerance)
{
    const castle::math::plane3d<double> plane(
        castle::math::point3d<double>(0.0, 0.0, 0.0),
        castle::math::vector3d<double>(0.0, 0.0, 1.0));

    EXPECT_TRUE(plane.contains(castle::math::point3d<double>(1.0, 2.0, 0.0)));
    EXPECT_FALSE(plane.contains(castle::math::point3d<double>(1.0, 2.0, 0.1)));
    EXPECT_TRUE(plane.contains(castle::math::point3d<double>(1.0, 2.0, 0.1), 0.1));
}

} // namespace
