#include "sample_support.hpp"

#include "castle/math/geometry.hpp"

int main()
{
    castle::math::point2d<int> a(1, 2);
    castle::math::point2d<int> b(4, 6);
    castle::math::vector2d<int> delta = b - a;
    castle::math::line2d<int> line(a, b);

    CASTLE_SAMPLE_CHECK(delta == castle::math::vector2d<int>(3, 4));
    CASTLE_SAMPLE_CHECK(line.point_at(2) == castle::math::point2d<int>(7, 10));
    return 0;
}
