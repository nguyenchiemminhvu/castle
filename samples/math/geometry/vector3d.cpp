#include "sample_support.hpp"

#include "castle/math/geometry/vector3d.hpp"

int main()
{
    castle::math::vector3d<int> x_axis(1, 0, 0);
    castle::math::vector3d<int> y_axis(0, 1, 0);
    castle::math::vector3d<int> z_axis = x_axis.cross(y_axis);

    CASTLE_SAMPLE_CHECK(z_axis == castle::math::vector3d<int>(0, 0, 1));
    CASTLE_SAMPLE_CHECK(x_axis.dot(y_axis) == 0);
    CASTLE_SAMPLE_CHECK(castle::math::scalar_triple_product(x_axis, y_axis, z_axis) == 1);
    CASTLE_SAMPLE_CHECK(castle::math::point3d<int>(1, 2, 3) + z_axis == castle::math::point3d<int>(1, 2, 4));

    castle::math::vector3d<float> unit = castle::math::vector3d<float>(0.0f, 0.0f, 2.0f).normalized();
    CASTLE_SAMPLE_CHECK(unit == castle::math::vector3d<float>(0.0f, 0.0f, 1.0f));
    CASTLE_SAMPLE_CHECK(castle::math::vector3d<float>(0.0f, 3.0f, 4.0f).length() == 5.0f);
    return 0;
}
