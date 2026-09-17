#include "sample_support.hpp"

#include "castle/math/geometry/algorithms.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::squared_distance(
        castle::math::point2d<int>(0, 0),
        castle::math::point2d<int>(3, 4)) == 25);

    CASTLE_SAMPLE_CHECK(castle::math::distance(
        castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
        castle::math::point3d<float>(1.0f, 2.0f, 2.0f)) == 3.0f);

    castle::math::point2d<float> projection = castle::math::nearest_point_on_segment(
        castle::math::point2d<float>(2.0f, 3.0f),
        castle::math::point2d<float>(0.0f, 0.0f),
        castle::math::point2d<float>(4.0f, 0.0f));
    CASTLE_SAMPLE_CHECK(projection == castle::math::point2d<float>(2.0f, 0.0f));

    CASTLE_SAMPLE_CHECK(castle::math::point_on_segment(
        castle::math::point2d<int>(2, 0),
        castle::math::point2d<int>(0, 0),
        castle::math::point2d<int>(4, 0)));

    castle::math::polygon2d<int, 4> square;
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(0, 0)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(3, 0)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(3, 3)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(0, 3)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::point_in_polygon(castle::math::point2d<int>(1, 1), square));
    CASTLE_SAMPLE_CHECK(!castle::math::point_in_polygon(castle::math::point2d<int>(4, 4), square));

    castle::container::array<castle::math::point2d<int>, 5U> input = {
        castle::math::point2d<int>(0, 0),
        castle::math::point2d<int>(2, 0),
        castle::math::point2d<int>(1, 1),
        castle::math::point2d<int>(2, 2),
        castle::math::point2d<int>(0, 2)
    };
    castle::math::polygon2d<int, 5U> hull;
    CASTLE_SAMPLE_CHECK(castle::math::convex_hull(input, hull) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(hull.size() == 4U);
    CASTLE_SAMPLE_CHECK(hull[0U] == castle::math::point2d<int>(0, 0));
    CASTLE_SAMPLE_CHECK(hull[1U] == castle::math::point2d<int>(2, 0));
    CASTLE_SAMPLE_CHECK(hull[2U] == castle::math::point2d<int>(2, 2));
    CASTLE_SAMPLE_CHECK(hull[3U] == castle::math::point2d<int>(0, 2));

    castle::container::array<castle::math::point2d<int>, 3U> candidates = {
        castle::math::point2d<int>(1, 1),
        castle::math::point2d<int>(4, 4),
        castle::math::point2d<int>(2, 2)
    };
    castle::math::nearest_point_result2d<int> nearest = castle::math::nearest_point(
        candidates,
        castle::math::point2d<int>(2, 3));
    CASTLE_SAMPLE_CHECK(nearest.found);
    CASTLE_SAMPLE_CHECK(nearest.index == 2U);
    CASTLE_SAMPLE_CHECK(nearest.point == castle::math::point2d<int>(2, 2));
    CASTLE_SAMPLE_CHECK(nearest.distance_squared == 1);
    return 0;
}
