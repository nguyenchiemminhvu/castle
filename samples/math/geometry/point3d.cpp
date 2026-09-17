#include "sample_support.hpp"

#include "castle/math/geometry/point3d.hpp"

int main()
{
    castle::math::point3d<int> point(1, 2, 3);
    CASTLE_SAMPLE_CHECK(point.x() == 1);
    CASTLE_SAMPLE_CHECK(point.y() == 2);
    CASTLE_SAMPLE_CHECK(point.z() == 3);

    point.set_x(4);
    point.set_y(5);
    point.set_z(6);
    CASTLE_SAMPLE_CHECK(point == castle::math::point3d<int>(4, 5, 6));
    CASTLE_SAMPLE_CHECK(point != castle::math::point3d<int>(6, 5, 4));
    return 0;
}
