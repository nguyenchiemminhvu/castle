#include "sample_support.hpp"

#include "castle/math/linalg/transform.hpp"

int main()
{
    const float epsilon = 1.0e-5F;

    const castle::math::matrix<float, 3U, 3U> transform_2d =
        castle::math::compose_transform_2d(
            castle::math::vector<float, 2U>(2.0F, 3.0F),
            castle::math::degrees_to_radians(90.0F),
            castle::math::vector<float, 2U>(2.0F, 1.0F));
    const castle::math::vector<float, 2U> point_2d =
        castle::math::transform_point_2d(
            transform_2d,
            castle::math::vector<float, 2U>(1.0F, 0.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point_2d[0U], 2.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point_2d[1U], 5.0F, epsilon));

    const castle::math::vector<float, 2U> direction_2d =
        castle::math::transform_vector_2d(
            transform_2d,
            castle::math::vector<float, 2U>(1.0F, 0.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(direction_2d[0U], 0.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(direction_2d[1U], 2.0F, epsilon));

    const castle::math::point2d<float> translated_point =
        castle::math::transform_point_2d(
            castle::math::translation_2d(2.0F, 3.0F),
            castle::math::point2d<float>(1.0F, 2.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(translated_point.x(), 3.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(translated_point.y(), 5.0F, epsilon));

    const castle::math::matrix<float, 4U, 4U> transform_3d =
        castle::math::compose_transform_3d(
            castle::math::vector<float, 3U>(1.0F, 2.0F, 3.0F),
            0.0F,
            0.0F,
            castle::math::degrees_to_radians(90.0F),
            castle::math::vector<float, 3U>(2.0F, 1.0F, 1.0F));
    const castle::math::vector<float, 3U> point_3d =
        castle::math::transform_point_3d(
            transform_3d,
            castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point_3d[0U], 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point_3d[1U], 4.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(point_3d[2U], 3.0F, epsilon));

    return 0;
}
