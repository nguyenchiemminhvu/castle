#include "sample_support.hpp"

#include "castle/math/linalg/quaternion.hpp"

int main()
{
    const float epsilon = 1.0e-5F;

    const castle::math::vector<float, 3U> input(1.0F, 0.0F, 0.0F);
    const castle::math::quaternion<float> identity;
    const castle::math::vector<float, 3U> identity_rotated = identity.rotate(input);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(identity_rotated, input, epsilon));

    const castle::math::quaternion<float> rotation =
        castle::math::quaternion<float>::from_axis_angle_degrees(
            castle::math::vector<float, 3U>(0.0F, 0.0F, 1.0F),
            90.0F);
    const castle::math::vector<float, 3U> rotated = rotation.rotate(input);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated[0U], 0.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated[1U], 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated[2U], 0.0F, epsilon));

    const castle::math::quaternion<float> scaled_rotation(
        rotation.w() * 2.0F,
        rotation.x() * 2.0F,
        rotation.y() * 2.0F,
        rotation.z() * 2.0F);
    const castle::math::vector<float, 3U> scaled_rotated = scaled_rotation.rotate(input);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(scaled_rotated, rotated, epsilon));

    const castle::math::matrix<float, 3U, 3U> rotation_matrix = rotation.to_matrix();
    const castle::math::vector<float, 3U> matrix_rotated = rotation_matrix * input;
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(matrix_rotated, rotated, epsilon));

    return 0;
}
