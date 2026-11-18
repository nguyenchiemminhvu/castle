#include "sample_support.hpp"

#include "castle/math/geometry/line2d.hpp"

int main()
{
    castle::math::line2d<int> line(castle::math::point2d<int>(1, 1), castle::math::point2d<int>(4, 5));

    CASTLE_SAMPLE_CHECK(line.origin() == castle::math::point2d<int>(1, 1));
    CASTLE_SAMPLE_CHECK(line.direction() == castle::math::vector2d<int>(3, 4));
    CASTLE_SAMPLE_CHECK(line.point_at(2) == castle::math::point2d<int>(7, 9));
    CASTLE_SAMPLE_CHECK(!line.degenerate());
    CASTLE_SAMPLE_CHECK(castle::math::line2d<int>().degenerate());
    return 0;
}
