#include "sample_support.hpp"

#include "castle/math/geometry/circle3d.hpp"

int main()
{
    castle::math::circle3d<float> circle(
        castle::math::point3d<float>(0.0f, 0.0f, 0.0f),
        castle::math::vector3d<float>(0.0f, 0.0f, 2.0f),
        5.0f);

    CASTLE_SAMPLE_CHECK(circle.radius() == 5.0f);
    CASTLE_SAMPLE_CHECK(circle.contains(castle::math::point3d<float>(3.0f, 4.0f, 0.0f)));
    CASTLE_SAMPLE_CHECK(circle.on_circle(castle::math::point3d<float>(3.0f, 4.0f, 0.0f)));
    CASTLE_SAMPLE_CHECK(circle.plane().contains(castle::math::point3d<float>(1.0f, 1.0f, 0.0f)));
    return 0;
}
