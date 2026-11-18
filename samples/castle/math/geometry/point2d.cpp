#include "sample_support.hpp"

#include "castle/math/geometry/point2d.hpp"

int main()
{
    castle::math::point2d<int> point(3, 4);
    CASTLE_SAMPLE_CHECK(point.x() == 3);
    CASTLE_SAMPLE_CHECK(point.y() == 4);

    point.set_x(5);
    point.set_y(6);
    CASTLE_SAMPLE_CHECK(point == castle::math::point2d<int>(5, 6));
    CASTLE_SAMPLE_CHECK(point != castle::math::point2d<int>(6, 5));
    return 0;
}
