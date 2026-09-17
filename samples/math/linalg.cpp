#include "sample_support.hpp"

#include "castle/math/linalg.hpp"

int main()
{
    const float epsilon = 1.0e-5F;

    const castle::math::quaternion<float> rotation =
        castle::math::quaternion<float>::from_axis_angle_degrees(
            castle::math::vector<float, 3U>(0.0F, 0.0F, 1.0F),
            90.0F);
    const castle::math::vector<float, 3U> rotated =
        rotation.rotate(castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated[0U], 0.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated[1U], 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated[2U], 0.0F, epsilon));

    const castle::math::vector<float, 2U> translated =
        castle::math::transform_point_2d(
            castle::math::translation_2d(2.0F, -1.0F),
            castle::math::vector<float, 2U>(1.0F, 3.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(translated[0U], 3.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(translated[1U], 2.0F, epsilon));

    return 0;
}
