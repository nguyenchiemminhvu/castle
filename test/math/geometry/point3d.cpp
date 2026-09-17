
#include <gtest/gtest.h>

#include "castle/math/geometry/point3d.h"

namespace
{

TEST(GeometryPoint3d, DefaultAndValueConstruction)
{
    constexpr castle::math::point3d<int> origin;
    constexpr castle::math::point3d<int> point(3, 4, -5);

    static_assert(origin.x() == 0 && origin.y() == 0 && origin.z() == 0, "default point");
    static_assert(point.x() == 3 && point.y() == 4 && point.z() == -5, "value point");

    EXPECT_EQ(point.x(), 3);
    EXPECT_EQ(point.y(), 4);
    EXPECT_EQ(point.z(), -5);
}

TEST(GeometryPoint3d, SettersAndComparison)
{
    castle::math::point3d<int> first;
    first.set_x(5);
    first.set_y(-2);
    first.set_z(7);

    const castle::math::point3d<int> same(5, -2, 7);
    const castle::math::point3d<int> different(5, -2, 6);

    EXPECT_TRUE(first == same);
    EXPECT_FALSE(first != same);
    EXPECT_FALSE(first == different);
    EXPECT_TRUE(first != different);
}

} // namespace
