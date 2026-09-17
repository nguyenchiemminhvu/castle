
#include <gtest/gtest.h>

#include "castle/math/geometry/line3d.h"

namespace
{

TEST(GeometryLine3d, DefaultAndConstructors)
{
    const castle::math::line3d<int> empty;
    EXPECT_TRUE(empty.degenerate());
    EXPECT_EQ(empty.origin(), castle::math::point3d<int>(0, 0, 0));
    EXPECT_EQ(empty.direction(), castle::math::vector3d<int>(0, 0, 0));

    const castle::math::point3d<int> a(1, 2, 3);
    const castle::math::point3d<int> b(4, 6, 8);

    const castle::math::line3d<int> by_vector(a, castle::math::vector3d<int>(3, 4, 5));
    const castle::math::line3d<int> by_points(a, b);

    EXPECT_EQ(by_vector.origin(), a);
    EXPECT_EQ(by_vector.direction(), castle::math::vector3d<int>(3, 4, 5));
    EXPECT_EQ(by_points.direction(), castle::math::vector3d<int>(3, 4, 5));
    EXPECT_FALSE(by_points.degenerate());
}

TEST(GeometryLine3d, PointAt)
{
    const castle::math::line3d<double> line(
        castle::math::point3d<double>(1.0, 2.0, 3.0),
        castle::math::vector3d<double>(2.0, -1.0, 4.0));

    EXPECT_EQ(line.point_at(0.0), castle::math::point3d<double>(1.0, 2.0, 3.0));
    EXPECT_EQ(line.point_at(2.0), castle::math::point3d<double>(5.0, 0.0, 11.0));
}

} // namespace
