#include "sample_support.hpp"

#include "castle/math/linalg/linalg.hpp"

int main()
{
    const float epsilon = 1.0e-5F;

    const castle::math::vector<float, 3U> diagonal_values(2.0F, 3.0F, 4.0F);
    const castle::math::matrix<float, 3U, 3U> diagonal =
        castle::math::diagonal_matrix(diagonal_values);
    const castle::math::vector<float, 3U> scaled =
        diagonal * castle::math::vector<float, 3U>(1.0F, 1.0F, 1.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(scaled[0U], 2.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(scaled[1U], 3.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(scaled[2U], 4.0F, epsilon));

    const castle::math::sin_cos_result<float> values =
        castle::math::sin_cos(castle::math::degrees_to_radians(90.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(values.sine, 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(values.cosine, 0.0F, epsilon));

    const castle::math::matrix<float, 4U, 4U> transform =
        castle::math::compose_transform_3d(
            castle::math::vector<float, 3U>(1.0F, 2.0F, 3.0F),
            0.0F,
            0.0F,
            castle::math::degrees_to_radians(90.0F),
            castle::math::vector<float, 3U>(2.0F, 1.0F, 1.0F));
    const castle::math::vector<float, 3U> point =
        castle::math::transform_point_3d(
            transform,
            castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point[0U], 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point[1U], 4.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point[2U], 3.0F, epsilon));

    return 0;
}
