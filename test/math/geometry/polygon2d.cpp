
#include <gtest/gtest.h>

#include "castle/math/geometry/polygon2d.h"

namespace
{

TEST(GeometryPolygon2d, CapacityAndMutationStatuses)
{
    castle::math::polygon2d<int, 2> polygon;

    EXPECT_EQ(polygon.size(), 0U);
    EXPECT_EQ(polygon.capacity(), 2U);
    EXPECT_TRUE(polygon.empty());
    EXPECT_FALSE(polygon.full());

    EXPECT_EQ(polygon.push_back(castle::math::point2d<int>(0, 0)), castle::status::ok);
    EXPECT_EQ(polygon.push_back(castle::math::point2d<int>(1, 0)), castle::status::ok);
    EXPECT_TRUE(polygon.full());
    EXPECT_EQ(polygon.push_back(castle::math::point2d<int>(2, 0)), castle::status::full);

    EXPECT_EQ(polygon.pop_back(), castle::status::ok);
    EXPECT_EQ(polygon.pop_back(), castle::status::ok);
    EXPECT_EQ(polygon.pop_back(), castle::status::empty);

    polygon.clear();
    EXPECT_TRUE(polygon.empty());
}

TEST(GeometryPolygon2d, AccessIteratorsAndArea)
{
    castle::math::polygon2d<int, 4> polygon;
    polygon.push_back(castle::math::point2d<int>(0, 0));
    polygon.push_back(castle::math::point2d<int>(4, 0));
    polygon.push_back(castle::math::point2d<int>(4, 3));
    polygon.push_back(castle::math::point2d<int>(0, 3));

    polygon[0] = castle::math::point2d<int>(0, 0);
    EXPECT_EQ(polygon[0], castle::math::point2d<int>(0, 0));
    EXPECT_EQ(polygon.data()[2], castle::math::point2d<int>(4, 3));
    EXPECT_EQ(polygon.signed_area2(), 24);
    EXPECT_FALSE(polygon.clockwise());

    volatile int runtime_extent = 3;
    castle::math::polygon2d<int, 3> runtime_polygon;
    runtime_polygon.push_back(castle::math::point2d<int>(0, 0));
    runtime_polygon.push_back(castle::math::point2d<int>(runtime_extent, 0));
    runtime_polygon.push_back(castle::math::point2d<int>(0, runtime_extent));
    const auto signed_area2 = &castle::math::polygon2d<int, 3>::signed_area2;
    EXPECT_EQ((runtime_polygon.*signed_area2)(), 9);

    int count = 0;
    for (auto it = polygon.begin(); it != polygon.end(); ++it)
    {
        ++count;
    }
    EXPECT_EQ(count, 4);

    castle::math::polygon2d<int, 4> clockwise;
    clockwise.push_back(castle::math::point2d<int>(0, 0));
    clockwise.push_back(castle::math::point2d<int>(0, 3));
    clockwise.push_back(castle::math::point2d<int>(4, 3));
    clockwise.push_back(castle::math::point2d<int>(4, 0));
    EXPECT_LT(clockwise.signed_area2(), 0);
    EXPECT_TRUE(clockwise.clockwise());
}

TEST(GeometryPolygon2d, DegenerateArea)
{
    castle::math::polygon2d<int, 1> single;
    EXPECT_EQ(single.signed_area2(), 0);
    single.push_back(castle::math::point2d<int>(0, 0));
    EXPECT_EQ(single.signed_area2(), 0);

    castle::math::polygon2d<int, 2> polygon;
    EXPECT_EQ(polygon.signed_area2(), 0);
    polygon.push_back(castle::math::point2d<int>(0, 0));
    EXPECT_EQ(polygon.signed_area2(), 0);
}

TEST(GeometryPolygon2d, FullStatusAcrossCapacities)
{
    castle::math::polygon2d<int, 1> one;
    EXPECT_EQ(one.push_back(castle::math::point2d<int>(0, 0)), castle::status::ok);
    EXPECT_EQ(one.push_back(castle::math::point2d<int>(1, 0)), castle::status::full);

    castle::math::polygon2d<int, 2> two;
    two.push_back(castle::math::point2d<int>(0, 0));
    two.push_back(castle::math::point2d<int>(1, 0));
    EXPECT_EQ(two.push_back(castle::math::point2d<int>(2, 0)), castle::status::full);

    castle::math::polygon2d<int, 3> three;
    three.push_back(castle::math::point2d<int>(0, 0));
    three.push_back(castle::math::point2d<int>(1, 0));
    three.push_back(castle::math::point2d<int>(2, 0));
    EXPECT_EQ(three.push_back(castle::math::point2d<int>(3, 0)), castle::status::full);

    castle::math::polygon2d<int, 4> four;
    four.push_back(castle::math::point2d<int>(0, 0));
    four.push_back(castle::math::point2d<int>(1, 0));
    four.push_back(castle::math::point2d<int>(2, 0));
    four.push_back(castle::math::point2d<int>(3, 0));
    EXPECT_EQ(four.push_back(castle::math::point2d<int>(4, 0)), castle::status::full);
}

TEST(GeometryPolygon2d, ConstAccess)
{
    castle::math::polygon2d<int, 3> mutable_polygon;
    mutable_polygon.push_back(castle::math::point2d<int>(1, 2));
    mutable_polygon.push_back(castle::math::point2d<int>(3, 4));

    const castle::math::polygon2d<int, 3>& polygon = mutable_polygon;
    EXPECT_EQ(polygon.size(), 2U);
    EXPECT_EQ(polygon[1], castle::math::point2d<int>(3, 4));
    EXPECT_EQ(polygon.data()[0], castle::math::point2d<int>(1, 2));
    EXPECT_EQ(polygon.begin(), polygon.data());
    EXPECT_EQ(polygon.end(), polygon.data() + polygon.size());
}

} // namespace
