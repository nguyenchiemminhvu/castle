
#include <gtest/gtest.h>

#include "castle/math/geometry/intersections.h"

namespace
{

template <typename T>
void expect_point2d_near(
    const castle::math::point2d<T>& actual,
    const castle::math::point2d<T>& expected)
{
    EXPECT_NEAR(actual.x(), expected.x(), static_cast<T>(1e-5));
    EXPECT_NEAR(actual.y(), expected.y(), static_cast<T>(1e-5));
}

template <typename T>
void expect_point3d_near(
    const castle::math::point3d<T>& actual,
    const castle::math::point3d<T>& expected)
{
    EXPECT_NEAR(actual.x(), expected.x(), static_cast<T>(1e-5));
    EXPECT_NEAR(actual.y(), expected.y(), static_cast<T>(1e-5));
    EXPECT_NEAR(actual.z(), expected.z(), static_cast<T>(1e-5));
}

TEST(GeometryIntersections, Line2dRelations)
{
    using namespace castle::math;

    point2d<double> intersection;

    const line2d<double> first(point2d<double>(0.0, 0.0), vector2d<double>(1.0, 0.0));
    const line2d<double> second(point2d<double>(0.0, 1.0), vector2d<double>(0.0, -1.0));

    EXPECT_EQ(intersect_lines(first, second, intersection), line2d_relation::intersecting);
    expect_point2d_near(intersection, point2d<double>(0.0, 0.0));

    const line2d<double> parallel(point2d<double>(0.0, 1.0), vector2d<double>(1.0, 0.0));
    EXPECT_EQ(intersect_lines(first, parallel, intersection), line2d_relation::parallel);

    const line2d<double> coincident(point2d<double>(2.0, 0.0), vector2d<double>(1.0, 0.0));
    EXPECT_EQ(intersect_lines(first, coincident, intersection), line2d_relation::coincident);
}

TEST(GeometryIntersections, LinePlaneRelations)
{
    using namespace castle::math;

    point3d<double> intersection;
    const plane3d<double> plane(
        point3d<double>(0.0, 0.0, 0.0),
        vector3d<double>(0.0, 0.0, 1.0));

    const line3d<double> crossing(
        point3d<double>(0.0, 0.0, 1.0),
        vector3d<double>(0.0, 0.0, -1.0));
    EXPECT_EQ(intersect_line_plane(crossing, plane, intersection),
              line_plane_relation::intersecting);
    expect_point3d_near(intersection, point3d<double>(0.0, 0.0, 0.0));

    const line3d<double> contained(
        point3d<double>(1.0, 0.0, 0.0),
        vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_plane(contained, plane, intersection),
              line_plane_relation::contained);

    const line3d<double> parallel(
        point3d<double>(0.0, 0.0, 1.0),
        vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_plane(parallel, plane, intersection),
              line_plane_relation::parallel);
}

TEST(GeometryIntersections, Line3dRelations)
{
    using namespace castle::math;

    point3d<double> intersection;
    const line3d<double> first(
        point3d<double>(0.0, 0.0, 0.0),
        vector3d<double>(1.0, 0.0, 0.0));

    const line3d<double> crossing(
        point3d<double>(1.0, -1.0, 0.0),
        vector3d<double>(0.0, 1.0, 0.0));
    EXPECT_EQ(intersect_lines(first, crossing, intersection),
              line3d_relation::intersecting);
    expect_point3d_near(intersection, point3d<double>(1.0, 0.0, 0.0));

    const line3d<double> coincident(
        point3d<double>(2.0, 0.0, 0.0),
        vector3d<double>(2.0, 0.0, 0.0));
    EXPECT_EQ(intersect_lines(first, coincident, intersection),
              line3d_relation::coincident);

    const line3d<double> parallel(
        point3d<double>(0.0, 1.0, 0.0),
        vector3d<double>(2.0, 0.0, 0.0));
    EXPECT_EQ(intersect_lines(first, parallel, intersection),
              line3d_relation::parallel);

    const line3d<double> skew(
        point3d<double>(0.0, 1.0, 1.0),
        vector3d<double>(0.0, 1.0, 0.0));
    EXPECT_EQ(intersect_lines(first, skew, intersection),
              line3d_relation::skew);
}

TEST(GeometryIntersections, PlaneRelations)
{
    using namespace castle::math;

    line3d<double> intersection;
    const plane3d<double> z0(
        point3d<double>(0.0, 0.0, 0.0),
        vector3d<double>(0.0, 0.0, 1.0));

    const plane3d<double> x0(
        point3d<double>(0.0, 0.0, 0.0),
        vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_planes(z0, x0, intersection), plane3d_relation::intersecting);
    expect_point3d_near(intersection.origin(), point3d<double>(0.0, 0.0, 0.0));
    EXPECT_NEAR(intersection.direction().y(), 1.0, 1e-12);

    const plane3d<double> z1(
        point3d<double>(0.0, 0.0, 1.0),
        vector3d<double>(0.0, 0.0, 1.0));
    EXPECT_EQ(intersect_planes(z0, z1, intersection), plane3d_relation::parallel);

    const plane3d<double> z2(
        point3d<double>(0.0, 0.0, 2.0),
        vector3d<double>(0.0, 0.0, 2.0));
    EXPECT_EQ(intersect_planes(z0, z2, intersection), plane3d_relation::parallel);

    const plane3d<double> same_plane(
        point3d<double>(1.0, 2.0, 0.0),
        vector3d<double>(0.0, 0.0, 2.0));
    EXPECT_EQ(intersect_planes(z0, same_plane, intersection),
              plane3d_relation::coincident);
}

TEST(GeometryIntersections, CircleCircle2dRelations)
{
    using namespace castle::math;

    circle2d<double> first(point2d<double>(0.0, 0.0), 2.0);
    circle2d<double> second(point2d<double>(2.0, 0.0), 2.0);

    auto two = intersect_circles(first, second);
    EXPECT_FALSE(two.coincident);
    EXPECT_EQ(two.count, 2U);

    auto tangent = intersect_circles(first,
        circle2d<double>(point2d<double>(4.0, 0.0), 2.0));
    EXPECT_EQ(tangent.count, 1U);
    expect_point2d_near(tangent.points[0], point2d<double>(2.0, 0.0));

    auto separate = intersect_circles(first,
        circle2d<double>(point2d<double>(5.0, 0.0), 2.0));
    EXPECT_EQ(separate.count, 0U);

    auto contained = intersect_circles(first,
        circle2d<double>(point2d<double>(0.5, 0.0), 0.25));
    EXPECT_EQ(contained.count, 0U);

    auto near_separate = intersect_circles(first,
        circle2d<double>(point2d<double>(4.05, 0.0), 2.0), 0.1);
    EXPECT_EQ(near_separate.count, 1U);

    auto concentric_different = intersect_circles(first,
        circle2d<double>(point2d<double>(0.0, 0.0), 1.0));
    EXPECT_EQ(concentric_different.count, 0U);

    auto coincident = intersect_circles(first, first);
    EXPECT_TRUE(coincident.coincident);
    EXPECT_EQ(coincident.count, 0U);
}

TEST(GeometryIntersections, LineCircle2dRelations)
{
    using namespace castle::math;

    const circle2d<double> circle(point2d<double>(0.0, 0.0), 2.0);

    const line2d<double> secant(
        point2d<double>(-5.0, 0.0), vector2d<double>(1.0, 0.0));
    auto two = intersect_line_circle(secant, circle);
    EXPECT_EQ(two.count, 2U);

    const line2d<double> tangent(
        point2d<double>(-5.0, 2.0), vector2d<double>(1.0, 0.0));
    auto one = intersect_line_circle(tangent, circle);
    EXPECT_EQ(one.count, 1U);

    const line2d<double> outside(
        point2d<double>(-5.0, 3.0), vector2d<double>(1.0, 0.0));
    auto none = intersect_line_circle(outside, circle);
    EXPECT_EQ(none.count, 0U);

    const line2d<double> degenerate_on(
        point2d<double>(2.0, 0.0), vector2d<double>(0.0, 0.0));
    auto point = intersect_line_circle(degenerate_on, circle);
    EXPECT_EQ(point.count, 1U);

    const line2d<double> degenerate_off(
        point2d<double>(3.0, 0.0), vector2d<double>(0.0, 0.0));
    auto no_point = intersect_line_circle(degenerate_off, circle);
    EXPECT_EQ(no_point.count, 0U);

    const line2d<double> near_outside(
        point2d<double>(0.0, 2.1), vector2d<double>(1.0, 0.0));
    auto tolerant = intersect_line_circle(near_outside, circle, 0.2);
    EXPECT_EQ(tolerant.count, 1U);
}

TEST(GeometryIntersections, LineCircle3dRelations)
{
    using namespace castle::math;

    const circle3d<double> circle(
        point3d<double>(0.0, 0.0, 0.0),
        vector3d<double>(0.0, 0.0, 1.0),
        2.0);

    const line3d<double> plane_crossing_hit(
        point3d<double>(2.0, 0.0, -1.0), vector3d<double>(0.0, 0.0, 1.0));
    auto hit = intersect_line_circle(plane_crossing_hit, circle);
    EXPECT_EQ(hit.count, 1U);

    const line3d<double> plane_crossing_miss(
        point3d<double>(0.0, 0.0, -1.0), vector3d<double>(0.0, 0.0, 1.0));
    auto miss = intersect_line_circle(plane_crossing_miss, circle);
    EXPECT_EQ(miss.count, 0U);

    const line3d<double> parallel(
        point3d<double>(0.0, 0.0, 1.0), vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_circle(parallel, circle).count, 0U);

    const line3d<double> degenerate_on(
        point3d<double>(2.0, 0.0, 0.0), vector3d<double>(0.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_circle(degenerate_on, circle).count, 1U);

    const line3d<double> degenerate_off(
        point3d<double>(3.0, 0.0, 0.0), vector3d<double>(0.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_circle(degenerate_off, circle).count, 0U);

    const line3d<double> contained_secant(
        point3d<double>(-5.0, 0.0, 0.0), vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_circle(contained_secant, circle).count, 2U);

    const line3d<double> contained_tangent(
        point3d<double>(-5.0, 2.0, 0.0), vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_circle(contained_tangent, circle).count, 1U);

    const line3d<double> contained_outside(
        point3d<double>(-5.0, 3.0, 0.0), vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_circle(contained_outside, circle).count, 0U);

    const line3d<double> contained_near_outside(
        point3d<double>(0.0, 2.1, 0.0), vector3d<double>(1.0, 0.0, 0.0));
    EXPECT_EQ(intersect_line_circle(contained_near_outside, circle, 0.2).count, 1U);
}

TEST(GeometryIntersections, CircleCircle3dRelations)
{
    using namespace castle::math;

    const circle3d<double> z_circle(
        point3d<double>(0.0, 0.0, 0.0),
        vector3d<double>(0.0, 0.0, 1.0),
        2.0);

    const circle3d<double> x_circle(
        point3d<double>(0.0, 1.0, 0.0),
        vector3d<double>(1.0, 0.0, 0.0),
        1.0);

    auto spatial = intersect_circles(z_circle, x_circle);
    EXPECT_FALSE(spatial.coincident);
    EXPECT_EQ(spatial.count, 1U);
    expect_point3d_near(spatial.points[0], point3d<double>(0.0, 2.0, 0.0));

    const circle3d<double> parallel(
        point3d<double>(0.0, 0.0, 1.0),
        vector3d<double>(0.0, 0.0, 1.0),
        2.0);
    EXPECT_EQ(intersect_circles(z_circle, parallel).count, 0U);

    const circle3d<double> same_y_plane(
        point3d<double>(1.0, 0.0, 0.0),
        vector3d<double>(0.0, 1.0, 0.0),
        2.0);
    const circle3d<double> same_y_plane_two(
        point3d<double>(-1.0, 0.0, 0.0),
        vector3d<double>(0.0, 1.0, 0.0),
        2.0);
    auto y_plane_two = intersect_circles(same_y_plane, same_y_plane_two);
    EXPECT_EQ(y_plane_two.count, 2U);

    const circle3d<double> same_x_plane(
        point3d<double>(0.0, 0.0, 0.0),
        vector3d<double>(1.0, 0.0, 0.0),
        2.0);
    const circle3d<double> same_x_plane_two(
        point3d<double>(0.0, 1.0, 0.0),
        vector3d<double>(1.0, 0.0, 0.0),
        2.0);
    auto x_plane_two = intersect_circles(same_x_plane, same_x_plane_two);
    EXPECT_EQ(x_plane_two.count, 2U);

    const circle3d<double> same_x_plane_disjoint(
        point3d<double>(0.0, 5.0, 0.0),
        vector3d<double>(1.0, 0.0, 0.0),
        1.0);
    EXPECT_EQ(intersect_circles(same_x_plane, same_x_plane_disjoint).count, 0U);

    const circle3d<double> same_x_plane_coincident = same_x_plane;
    auto coincident = intersect_circles(same_x_plane, same_x_plane_coincident);
    EXPECT_TRUE(coincident.coincident);
    EXPECT_EQ(coincident.count, 0U);
}

} // namespace
