
#include <gtest/gtest.h>

#include "castle/math/geometry/line2d.h"

namespace
{

TEST(GeometryLine2d, DefaultAndConstructors)
{
    const castle::math::line2d<int> empty;
    EXPECT_TRUE(empty.degenerate());
    EXPECT_EQ(empty.origin(), castle::math::point2d<int>(0, 0));
    EXPECT_EQ(empty.direction(), castle::math::vector2d<int>(0, 0));

    const castle::math::point2d<int> a(1, 2);
    const castle::math::point2d<int> b(4, 6);

    const castle::math::line2d<int> by_vector(a, castle::math::vector2d<int>(3, 4));
    const castle::math::line2d<int> by_points(a, b);

    EXPECT_EQ(by_vector.origin(), a);
    EXPECT_EQ(by_vector.direction(), castle::math::vector2d<int>(3, 4));
    EXPECT_EQ(by_points.direction(), castle::math::vector2d<int>(3, 4));
    EXPECT_FALSE(by_points.degenerate());
}

TEST(GeometryLine2d, PointAt)
{
    const castle::math::line2d<double> line(
        castle::math::point2d<double>(1.0, 2.0),
        castle::math::vector2d<double>(2.0, -1.0));

    EXPECT_EQ(line.point_at(0.0), castle::math::point2d<double>(1.0, 2.0));
    EXPECT_EQ(line.point_at(2.0), castle::math::point2d<double>(5.0, 0.0));
}

} // namespace
