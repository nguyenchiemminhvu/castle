#include "sample_support.hpp"

#include "castle/math/geometry/geometry.hpp"

int main()
{
    castle::math::circle2d<float> circle(castle::math::point2d<float>(0.0f, 0.0f), 5.0f);
    castle::math::line2d<float> line(castle::math::point2d<float>(-6.0f, 0.0f), castle::math::vector2d<float>(1.0f, 0.0f));
    castle::math::intersection_points2d<float> hits = castle::math::intersect_line_circle(line, circle);

    castle::math::polygon2d<int, 4> square;
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(0, 0)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(2, 0)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(2, 2)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(square.push_back(castle::math::point2d<int>(0, 2)) == castle::status::ok);

    CASTLE_SAMPLE_CHECK(hits.count == 2U);
    CASTLE_SAMPLE_CHECK(hits.points[0U] == castle::math::point2d<float>(5.0f, 0.0f));
    CASTLE_SAMPLE_CHECK(hits.points[1U] == castle::math::point2d<float>(-5.0f, 0.0f));
    CASTLE_SAMPLE_CHECK(castle::math::point_in_polygon(castle::math::point2d<int>(1, 1), square));
    return 0;
}
