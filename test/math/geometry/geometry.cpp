
#include <gtest/gtest.h>

#include "castle/math/geometry.h"

namespace
{

TEST(GeometryUmbrella, AggregateHeadersAreReachable)
{
    constexpr castle::math::point2d<int> p2(1, 2);
    constexpr castle::math::point3d<int> p3(1, 2, 3);
    constexpr castle::math::vector2d<int> v2(3, 4);
    constexpr castle::math::vector3d<int> v3(1, 0, 0);

    static_assert(p2.x() == 1 && p2.y() == 2, "point2d");
    static_assert(p3.z() == 3, "point3d");
    static_assert(v2.squared_length() == 25, "vector2d");
    static_assert(v3.squared_length() == 1, "vector3d");

    castle::math::line2d<int> line2(p2, v2);
    castle::math::line3d<int> line3(p3, v3);
    castle::math::plane3d<int> plane(
        p3, castle::math::vector3d<int>(0, 0, 1));
    castle::math::polygon2d<int, 4> polygon;
    polygon.push_back(p2);

    EXPECT_EQ(line2.point_at(1), castle::math::point2d<int>(4, 6));
    EXPECT_EQ(line3.point_at(1), castle::math::point3d<int>(2, 2, 3));
    EXPECT_EQ(plane.normal(), castle::math::vector3d<int>(0, 0, 1));
    EXPECT_EQ(polygon.size(), 1U);

    SUCCEED();
}

TEST(GeometryUmbrella, FloatingTypesReachable)
{
    const castle::math::circle2d<float> circle2(
        castle::math::point2d<float>(0.0f, 0.0f), 1.0f);
    const castle::math::circle3d<float> circle3(
        castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
        castle::math::vector3d<float>(0.0f, 0.0f, 1.0f),
        1.0f);

    EXPECT_TRUE(circle2.contains(castle::math::point2d<float>(0.0f, 0.0f)));
    EXPECT_TRUE(circle3.contains(castle::math::point3d<float>(0.0f, 0.0f, 0.0f)));
}

} // namespace
