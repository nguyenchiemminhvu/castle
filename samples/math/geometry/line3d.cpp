#include "sample_support.hpp"

#include "castle/math/geometry/line3d.hpp"

int main()
{
    castle::math::line3d<float> line(
        castle::math::point3d<float>(1.0f, 2.0f, 3.0f),
        castle::math::vector3d<float>(0.0f, 0.0f, 2.0f));

    CASTLE_SAMPLE_CHECK(line.origin() == castle::math::point3d<float>(1.0f, 2.0f, 3.0f));
    CASTLE_SAMPLE_CHECK(line.direction() == castle::math::vector3d<float>(0.0f, 0.0f, 2.0f));
    CASTLE_SAMPLE_CHECK(line.point_at(1.5f) == castle::math::point3d<float>(1.0f, 2.0f, 6.0f));
    CASTLE_SAMPLE_CHECK(!line.degenerate());
    CASTLE_SAMPLE_CHECK(castle::math::line3d<int>().degenerate());
    return 0;
}
