#include "sample_support.hpp"

#include "castle/math/geometry/plane3d.hpp"

int main()
{
    castle::math::plane3d<int> plane(
        castle::math::point3d<int>(0, 0, 1),
        castle::math::vector3d<int>(0, 0, 2));

    CASTLE_SAMPLE_CHECK(plane.point() == castle::math::point3d<int>(0, 0, 1));
    CASTLE_SAMPLE_CHECK(plane.normal() == castle::math::vector3d<int>(0, 0, 2));
    CASTLE_SAMPLE_CHECK(plane.signed_value(castle::math::point3d<int>(0, 0, 3)) == 4);
    CASTLE_SAMPLE_CHECK(plane.contains(castle::math::point3d<int>(2, 3, 1)));
    CASTLE_SAMPLE_CHECK(!plane.contains(castle::math::point3d<int>(0, 0, 2)));
    return 0;
}
