
#include <gtest/gtest.h>

#include "castle/math/geometry/algorithms.h"

namespace
{

using castle::math::point2d;
using castle::math::point3d;

point2d<int> runtime_point2d(int x, int y)
{
    volatile int runtime_x = x;
    volatile int runtime_y = y;
    return point2d<int>(runtime_x, runtime_y);
}

castle::size_type runtime_size(castle::size_type value)
{
    volatile castle::size_type runtime_value = value;
    return runtime_value;
}

bool runtime_point_on_segment(const point2d<int>& point,
                              const point2d<int>& first,
                              const point2d<int>& second,
                              int epsilon)
{
    return castle::math::point_on_segment(point, first, second, epsilon);
}

bool runtime_point_in_polygon(const point2d<int>& point,
                              const point2d<int>* vertices,
                              castle::size_type count,
                              int epsilon)
{
    return castle::math::point_in_polygon(point, vertices, count, epsilon);
}

void runtime_heap_sift_down(castle::container::array<point2d<int>, 4>& values,
                            castle::size_type start,
                            castle::size_type count)
{
    castle::math::detail::heap_sift_down(values, start, count);
}

void runtime_heap_sort(castle::container::array<point2d<int>, 4>& values,
                       castle::size_type count)
{
    castle::math::detail::heap_sort(values, count);
}

TEST(GeometryAlgorithms, SquaredDistanceAndDistance)
{
    constexpr point2d<int> a2(0, 0);
    constexpr point2d<int> b2(3, 4);
    static_assert(castle::math::squared_distance(a2, b2) == 25, "2d squared distance");

    constexpr point3d<int> a3(0, 0, 0);
    constexpr point3d<int> b3(1, 2, 2);
    static_assert(castle::math::squared_distance(a3, b3) == 9, "3d squared distance");

    EXPECT_NEAR(castle::math::distance(point2d<double>(0.0, 0.0), point2d<double>(3.0, 4.0)),
                5.0, 1e-12);
    EXPECT_NEAR(castle::math::distance(point3d<double>(0.0, 0.0, 0.0),
                                        point3d<double>(1.0, 2.0, 2.0)),
                3.0, 1e-12);
}

TEST(GeometryAlgorithms, NearestPointOnLines)
{
    const castle::math::line2d<double> line2(
        point2d<double>(0.0, 0.0),
        castle::math::vector2d<double>(1.0, 0.0));
    EXPECT_EQ(castle::math::nearest_point(point2d<double>(3.0, 4.0), line2),
              point2d<double>(3.0, 0.0));

    const castle::math::line2d<double> degenerate2;
    EXPECT_EQ(castle::math::nearest_point(point2d<double>(3.0, 4.0), degenerate2),
              point2d<double>(0.0, 0.0));

    const castle::math::line3d<double> line3(
        point3d<double>(0.0, 0.0, 0.0),
        castle::math::vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(castle::math::nearest_point(point3d<double>(3.0, 4.0, 5.0), line3),
              point3d<double>(3.0, 0.0, 0.0));

    const castle::math::line3d<double> degenerate3;
    EXPECT_EQ(castle::math::nearest_point(point3d<double>(3.0, 4.0, 5.0), degenerate3),
              point3d<double>(0.0, 0.0, 0.0));
}

TEST(GeometryAlgorithms, NearestPointOnSegments)
{
    const point2d<double> first2(0.0, 0.0);
    const point2d<double> second2(10.0, 0.0);
    EXPECT_EQ(castle::math::nearest_point_on_segment(point2d<double>(5.0, 3.0), first2, second2),
              point2d<double>(5.0, 0.0));
    EXPECT_EQ(castle::math::nearest_point_on_segment(point2d<double>(-2.0, 3.0), first2, second2),
              first2);
    EXPECT_EQ(castle::math::nearest_point_on_segment(point2d<double>(12.0, 3.0), first2, second2),
              second2);
    EXPECT_EQ(castle::math::nearest_point_on_segment(point2d<double>(4.0, 3.0), first2, first2),
              first2);

    const point3d<double> first3(0.0, 0.0, 0.0);
    const point3d<double> second3(0.0, 0.0, 10.0);
    EXPECT_EQ(castle::math::nearest_point_on_segment(point3d<double>(1.0, 2.0, 5.0), first3, second3),
              point3d<double>(0.0, 0.0, 5.0));
    EXPECT_EQ(castle::math::nearest_point_on_segment(point3d<double>(1.0, 2.0, -2.0), first3, second3),
              first3);
    EXPECT_EQ(castle::math::nearest_point_on_segment(point3d<double>(1.0, 2.0, 12.0), first3, second3),
              second3);
    EXPECT_EQ(castle::math::nearest_point_on_segment(point3d<double>(1.0, 2.0, 5.0), first3, first3),
              first3);
}

TEST(GeometryAlgorithms, NearestPointInFixedArrays)
{
    castle::container::array<point2d<int>, 3> points2;
    points2[0] = point2d<int>(10, 10);
    points2[1] = point2d<int>(2, 2);
    points2[2] = point2d<int>(5, 5);
    const auto result2 = castle::math::nearest_point(points2, point2d<int>(3, 3));
    EXPECT_TRUE(result2.found);
    EXPECT_EQ(result2.index, 1U);
    EXPECT_EQ(result2.point, point2d<int>(2, 2));
    EXPECT_EQ(result2.distance_squared, 2);

    castle::container::array<point3d<int>, 3> points3;
    points3[0] = point3d<int>(10, 10, 10);
    points3[1] = point3d<int>(1, 2, 3);
    points3[2] = point3d<int>(5, 5, 5);
    const auto result3 = castle::math::nearest_point(points3, point3d<int>(2, 2, 4));
    EXPECT_TRUE(result3.found);
    EXPECT_EQ(result3.index, 1U);
    EXPECT_EQ(result3.point, point3d<int>(1, 2, 3));
    EXPECT_EQ(result3.distance_squared, 2);
}

TEST(GeometryAlgorithms, PointOnSegment)
{
    EXPECT_TRUE(castle::math::point_on_segment(
        point2d<int>(2, 0), point2d<int>(0, 0), point2d<int>(4, 0)));
    EXPECT_FALSE(castle::math::point_on_segment(
        point2d<int>(2, 1), point2d<int>(0, 0), point2d<int>(4, 0)));
    EXPECT_TRUE(castle::math::point_on_segment(
        point2d<int>(5, 0), point2d<int>(0, 0), point2d<int>(4, 0), 4));
    EXPECT_FALSE(castle::math::point_on_segment(
        point2d<int>(6, 0), point2d<int>(0, 0), point2d<int>(4, 0), 1));

    EXPECT_TRUE(runtime_point_on_segment(
        runtime_point2d(0, 0), runtime_point2d(0, 0), runtime_point2d(0, 0), 0));
    EXPECT_FALSE(runtime_point_on_segment(
        runtime_point2d(1, 0), runtime_point2d(0, 0), runtime_point2d(0, 0), 0));

    const point2d<double> floating_point(1.0, 0.0);
    const point2d<double> floating_end(0.0, 0.0);
    EXPECT_TRUE(castle::math::point_on_segment(
        floating_end, floating_end, floating_end));
    EXPECT_FALSE(castle::math::point_on_segment(
        floating_point, floating_end, floating_end));
    EXPECT_TRUE(castle::math::point_on_segment(
        point2d<double>(2.0, 0.0), point2d<double>(0.0, 0.0),
        point2d<double>(4.0, 0.0)));
}

TEST(GeometryAlgorithms, PointInPolygonArrayAndDegenerate)
{
    const point2d<int> square[4] = {
        point2d<int>(0, 0), point2d<int>(4, 0),
        point2d<int>(4, 4), point2d<int>(0, 4)
    };

    EXPECT_FALSE(runtime_point_in_polygon(
        runtime_point2d(2, 2), square, runtime_size(0U), 0));
    EXPECT_FALSE(runtime_point_in_polygon(
        runtime_point2d(2, 2), static_cast<const point2d<int>*>(nullptr), runtime_size(4U), 0));

    const point2d<double> floating_square[4] = {
        point2d<double>(0.0, 0.0), point2d<double>(4.0, 0.0),
        point2d<double>(4.0, 4.0), point2d<double>(0.0, 4.0)
    };
    EXPECT_TRUE(castle::math::point_in_polygon(
        point2d<double>(2.0, 2.0), floating_square, 4U));
    EXPECT_FALSE(castle::math::point_in_polygon(
        point2d<double>(5.0, 2.0), floating_square, 4U));
    EXPECT_FALSE(castle::math::point_in_polygon(
        point2d<double>(2.0, 2.0), floating_square, 0U));
    EXPECT_TRUE(castle::math::point_in_polygon(
        point2d<double>(0.0, 2.0), floating_square, 4U));
    EXPECT_TRUE(castle::math::point_in_polygon(point2d<int>(2, 2), square, 4U));
    EXPECT_FALSE(castle::math::point_in_polygon(point2d<int>(5, 2), square, 4U));
    EXPECT_TRUE(castle::math::point_in_polygon(point2d<int>(0, 2), square, 4U));
    EXPECT_TRUE(castle::math::point_on_segment(point2d<int>(2, 0), square[0], square[1]));

    castle::math::polygon2d<int, 4> polygon;
    for (const auto& point : square)
    {
        polygon.push_back(point);
    }
    EXPECT_TRUE(castle::math::point_in_polygon(point2d<int>(2, 2), polygon));
    EXPECT_FALSE(castle::math::point_in_polygon(point2d<int>(5, 2), polygon));
}

TEST(GeometryAlgorithms, DetailPrimitivesAndHeapBranches)
{
    using castle::math::detail::orientation;
    using castle::math::detail::point_less;

    EXPECT_TRUE(point_less(point2d<int>(0, 0), point2d<int>(1, 0)));
    EXPECT_FALSE(point_less(point2d<int>(1, 0), point2d<int>(0, 0)));
    EXPECT_TRUE(point_less(point2d<int>(1, 0), point2d<int>(1, 1)));
    EXPECT_FALSE(point_less(point2d<int>(1, 1), point2d<int>(1, 1)));

    EXPECT_GT(orientation(point2d<int>(0, 0), point2d<int>(2, 0), point2d<int>(0, 2)), 0);
    EXPECT_LT(orientation(point2d<int>(0, 0), point2d<int>(0, 2), point2d<int>(2, 0)), 0);
    EXPECT_EQ(orientation(point2d<int>(0, 0), point2d<int>(1, 1), point2d<int>(2, 2)), 0);

    castle::container::array<point2d<int>, 1> one;
    one[0] = point2d<int>(1, 1);
    castle::math::detail::heap_sort(one, 1U);
    EXPECT_EQ(one[0], point2d<int>(1, 1));

    castle::container::array<point2d<int>, 4> heap = {
        runtime_point2d(1, 1), runtime_point2d(4, 0),
        runtime_point2d(3, 3), runtime_point2d(2, 2)
    };
    runtime_heap_sift_down(heap, 0U, 4U);
    EXPECT_FALSE(point_less(heap[0], heap[1]));

    runtime_heap_sort(heap, 4U);
    EXPECT_EQ(heap[0], point2d<int>(1, 1));
    EXPECT_EQ(heap[3], point2d<int>(4, 0));

    castle::container::array<point2d<int>, 4> sorted = {
        runtime_point2d(0, 0), runtime_point2d(0, 0),
        runtime_point2d(1, 0), runtime_point2d(1, 0)
    };
    castle::container::array<point2d<int>, 4> unique;
    castle::size_type unique_count = 0U;
    castle::math::detail::copy_unique(sorted, 4U, unique, unique_count);
    EXPECT_EQ(unique_count, 2U);
    EXPECT_EQ(unique[0], point2d<int>(0, 0));
    EXPECT_EQ(unique[1], point2d<int>(1, 0));
}

TEST(GeometryAlgorithms, ConvexHull)
{
    castle::container::array<point2d<int>, 1> single = { point2d<int>(2, 3) };
    castle::math::polygon2d<int, 1> single_hull;
    EXPECT_EQ(castle::math::convex_hull(single, single_hull), castle::status::ok);
    ASSERT_EQ(single_hull.size(), 1U);
    EXPECT_EQ(single_hull[0], point2d<int>(2, 3));

    castle::container::array<point2d<int>, 2> pair = {
        point2d<int>(2, 2), point2d<int>(0, 0)
    };
    castle::math::polygon2d<int, 2> pair_hull;
    EXPECT_EQ(castle::math::convex_hull(pair, pair_hull), castle::status::ok);
    ASSERT_EQ(pair_hull.size(), 2U);
    EXPECT_EQ(pair_hull[0], point2d<int>(0, 0));
    EXPECT_EQ(pair_hull[1], point2d<int>(2, 2));

    castle::container::array<point2d<int>, 5> duplicates = {
        point2d<int>(0, 0), point2d<int>(1, 1), point2d<int>(0, 0),
        point2d<int>(2, 2), point2d<int>(3, 3)
    };
    castle::math::polygon2d<int, 5> duplicate_hull;
    EXPECT_EQ(castle::math::convex_hull(duplicates, duplicate_hull), castle::status::ok);
    EXPECT_EQ(duplicate_hull.size(), 2U);

    castle::container::array<point2d<int>, 8> points = {
        point2d<int>(0, 0), point2d<int>(4, 0),
        point2d<int>(4, 4), point2d<int>(0, 4),
        point2d<int>(2, 2), point2d<int>(1, 1),
        point2d<int>(4, 2), point2d<int>(0, 0)
    };
    castle::math::polygon2d<int, 8> hull;
    EXPECT_EQ(castle::math::convex_hull(points, hull), castle::status::ok);
    ASSERT_EQ(hull.size(), 4U);
    EXPECT_EQ(hull[0], point2d<int>(0, 0));
    EXPECT_EQ(hull[1], point2d<int>(4, 0));
    EXPECT_EQ(hull[2], point2d<int>(4, 4));
    EXPECT_EQ(hull[3], point2d<int>(0, 4));

    castle::container::array<point2d<int>, 4> collinear = {
        point2d<int>(0, 0), point2d<int>(1, 0),
        point2d<int>(2, 0), point2d<int>(3, 0)
    };
    castle::math::polygon2d<int, 4> collinear_hull;
    EXPECT_EQ(castle::math::convex_hull(collinear, collinear_hull), castle::status::ok);
    EXPECT_EQ(collinear_hull.size(), 2U);
    EXPECT_EQ(collinear_hull[0], point2d<int>(0, 0));
    EXPECT_EQ(collinear_hull[1], point2d<int>(3, 0));
}

} // namespace
