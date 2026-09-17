#include "sample_support.hpp"

#include "castle/core/constants.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/linalg/trigonometry.hpp"

int main()
{
    const float epsilon = 1.0e-5F;

    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::sin(0.0F), 0.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cos(0.0F), 1.0F, epsilon));

    const castle::math::sin_cos_result<float> right_angle =
        castle::math::sin_cos(castle::math::pi<float>() * 0.5F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(right_angle.sine, 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(right_angle.cosine, 0.0F, epsilon));

    const float thirty_degrees = castle::math::degrees_to_radians(30.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::radians_sin(thirty_degrees),
        0.5F,
        epsilon));

    return 0;
}
