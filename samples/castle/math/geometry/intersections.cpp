#include "sample_support.hpp"

#include "castle/math/geometry/intersections.hpp"

int main()
{
    castle::math::point2d<float> line_hit;
    castle::math::line2d_relation line_relation = castle::math::intersect_lines(
        castle::math::line2d<float>(castle::math::point2d<float>(0.0f, 0.0f), castle::math::vector2d<float>(1.0f, 0.0f)),
        castle::math::line2d<float>(castle::math::point2d<float>(0.0f, -1.0f), castle::math::vector2d<float>(0.0f, 1.0f)),
        line_hit);
    CASTLE_SAMPLE_CHECK(line_relation == castle::math::line2d_relation::intersecting);
    CASTLE_SAMPLE_CHECK(line_hit == castle::math::point2d<float>(0.0f, 0.0f));

    castle::math::point3d<float> plane_hit;
    castle::math::line_plane_relation plane_relation = castle::math::intersect_line_plane(
        castle::math::line3d<float>(castle::math::point3d<float>(0.0f, 0.0f, 0.0f), castle::math::vector3d<float>(0.0f, 0.0f, 1.0f)),
        castle::math::plane3d<float>(castle::math::point3d<float>(0.0f, 0.0f, 2.0f), castle::math::vector3d<float>(0.0f, 0.0f, 1.0f)),
        plane_hit);
    CASTLE_SAMPLE_CHECK(plane_relation == castle::math::line_plane_relation::intersecting);
    CASTLE_SAMPLE_CHECK(plane_hit == castle::math::point3d<float>(0.0f, 0.0f, 2.0f));

    castle::math::intersection_points2d<float> circle_hits = castle::math::intersect_line_circle(
        castle::math::line2d<float>(castle::math::point2d<float>(-10.0f, 0.0f), castle::math::vector2d<float>(1.0f, 0.0f)),
        castle::math::circle2d<float>(castle::math::point2d<float>(0.0f, 0.0f), 5.0f));
    CASTLE_SAMPLE_CHECK(circle_hits.count == 2U);
    CASTLE_SAMPLE_CHECK(circle_hits.points[0U] == castle::math::point2d<float>(5.0f, 0.0f));
    CASTLE_SAMPLE_CHECK(circle_hits.points[1U] == castle::math::point2d<float>(-5.0f, 0.0f));
    return 0;
}
