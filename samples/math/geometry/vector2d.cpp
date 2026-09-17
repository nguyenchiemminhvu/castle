#include "sample_support.hpp"

#include "castle/math/geometry/vector2d.hpp"

int main()
{
    castle::math::vector2d<int> vector(3, 4);
    CASTLE_SAMPLE_CHECK(vector.x() == 3);
    CASTLE_SAMPLE_CHECK(vector.y() == 4);
    CASTLE_SAMPLE_CHECK(vector.squared_length() == 25);
    CASTLE_SAMPLE_CHECK(vector.dot(castle::math::vector2d<int>(1, 2)) == 11);
    CASTLE_SAMPLE_CHECK(vector.perpendicular() == castle::math::vector2d<int>(-4, 3));
    CASTLE_SAMPLE_CHECK(vector.cross(castle::math::vector2d<int>(1, 2)) == 2);
    CASTLE_SAMPLE_CHECK(2 * vector == castle::math::vector2d<int>(6, 8));
    CASTLE_SAMPLE_CHECK(castle::math::point2d<int>(1, 1) + vector == castle::math::point2d<int>(4, 5));

    castle::math::vector2d<float> unit = castle::math::vector2d<float>(4.0f, 0.0f).normalized();
    CASTLE_SAMPLE_CHECK(unit == castle::math::vector2d<float>(1.0f, 0.0f));
    return 0;
}
