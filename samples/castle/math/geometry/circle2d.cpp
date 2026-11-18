#include "sample_support.hpp"

#include "castle/math/geometry/circle2d.hpp"

int main()
{
    castle::math::circle2d<float> circle(castle::math::point2d<float>(1.0f, 2.0f), 5.0f);

    CASTLE_SAMPLE_CHECK(circle.center() == castle::math::point2d<float>(1.0f, 2.0f));
    CASTLE_SAMPLE_CHECK(circle.radius() == 5.0f);
    CASTLE_SAMPLE_CHECK(circle.contains(castle::math::point2d<float>(4.0f, 6.0f)));
    CASTLE_SAMPLE_CHECK(circle.on_circle(castle::math::point2d<float>(4.0f, 6.0f)));
    CASTLE_SAMPLE_CHECK(!circle.on_circle(castle::math::point2d<float>(1.0f, 2.0f)));
    return 0;
}
