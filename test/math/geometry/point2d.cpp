
#include <gtest/gtest.h>

#include "castle/math/geometry/point2d.h"

namespace
{

TEST(GeometryPoint2d, DefaultAndValueConstruction)
{
    constexpr castle::math::point2d<int> origin;
    constexpr castle::math::point2d<int> point(3, 4);

    static_assert(origin.x() == 0 && origin.y() == 0, "default point");
    static_assert(point.x() == 3 && point.y() == 4, "value point");

    EXPECT_EQ(point.x(), 3);
    EXPECT_EQ(point.y(), 4);
}

TEST(GeometryPoint2d, SettersAndComparison)
{
    castle::math::point2d<int> first;
    first.set_x(5);
    first.set_y(-2);

    const castle::math::point2d<int> same(5, -2);
    const castle::math::point2d<int> different(5, -1);

    EXPECT_TRUE(first == same);
    EXPECT_FALSE(first != same);
    EXPECT_FALSE(first == different);
    EXPECT_TRUE(first != different);
}

} // namespace
