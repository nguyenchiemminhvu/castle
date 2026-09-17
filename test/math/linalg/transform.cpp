#include <gtest/gtest.h>

#include "castle/math/linalg/transform.h"

TEST(Transform, TwoDimensionalTranslationAndScaling)
{
    const castle::math::matrix<float, 3U, 3U> translation =
        castle::math::translation_2d(3.0F, 4.0F);
    const castle::math::matrix<float, 3U, 3U> scaling =
        castle::math::scaling_2d(2.0F, 3.0F);

    EXPECT_EQ(translation(0U, 2U), 3.0F);
    EXPECT_EQ(translation(1U, 2U), 4.0F);
    EXPECT_EQ(scaling(0U, 0U), 2.0F);
    EXPECT_EQ(scaling(1U, 1U), 3.0F);

    const castle::math::vector<float, 2U> point(1.0F, 2.0F);
    const castle::math::vector<float, 2U> translated =
        castle::math::transform_point_2d(translation, point);
    const castle::math::vector<float, 2U> scaled =
        castle::math::transform_point_2d(scaling, point);

    const castle::math::vector<float, 2U> translated_expected(4.0F, 6.0F);
    const castle::math::vector<float, 2U> scaled_expected(2.0F, 6.0F);
    EXPECT_EQ(translated, translated_expected);
    EXPECT_EQ(scaled, scaled_expected);
}

TEST(Transform, TwoDimensionalRotationAndVectorSemantics)
{
    const castle::math::matrix<float, 3U, 3U> rotation =
        castle::math::rotation_2d_degrees(90.0F);
    const castle::math::vector<float, 2U> point(1.0F, 0.0F);
    const castle::math::vector<float, 2U> direction(1.0F, 0.0F);

    EXPECT_NEAR(
        castle::math::transform_point_2d(rotation, point)[0U],
        0.0F,
        1.0e-5F);
    EXPECT_NEAR(
        castle::math::transform_point_2d(rotation, point)[1U],
        1.0F,
        1.0e-5F);
    EXPECT_NEAR(
        castle::math::transform_vector_2d(rotation, direction)[0U],
        0.0F,
        1.0e-5F);
    EXPECT_NEAR(
        castle::math::transform_vector_2d(rotation, direction)[1U],
        1.0F,
        1.0e-5F);
}

TEST(Transform, ThreeDimensionalTranslationScalingAndAxisRotation)
{
    const castle::math::matrix<double, 4U, 4U> translation =
        castle::math::translation_3d(10.0, 20.0, 30.0);
    const castle::math::matrix<double, 4U, 4U> scaling =
        castle::math::scaling_3d(2.0, 3.0, 4.0);
    const castle::math::matrix<double, 4U, 4U> rotation =
        castle::math::rotation_z_degrees(90.0);

    const castle::math::vector<double, 3U> point(1.0, 2.0, 3.0);
    const castle::math::vector<double, 3U> translated =
        castle::math::transform_point_3d(translation, point);
    const castle::math::vector<double, 3U> scaled =
        castle::math::transform_point_3d(scaling, point);
    const castle::math::vector<double, 3U> rotated =
        castle::math::transform_point_3d(rotation, castle::math::vector<double, 3U>(1.0, 0.0, 0.0));

    const castle::math::vector<double, 3U> translated_expected(11.0, 22.0, 33.0);
    const castle::math::vector<double, 3U> scaled_expected(2.0, 6.0, 12.0);
    EXPECT_EQ(translated, translated_expected);
    EXPECT_EQ(scaled, scaled_expected);
    EXPECT_NEAR(rotated[0U], 0.0, 1.0e-10);
    EXPECT_NEAR(rotated[1U], 1.0, 1.0e-10);
    EXPECT_NEAR(rotated[2U], 0.0, 1.0e-10);
}

TEST(Transform, AxisAngleAndRotationComposition)
{
    const castle::math::matrix<double, 4U, 4U> axis_rotation =
        castle::math::rotation_axis_angle_degrees(
            castle::math::vector<double, 3U>(0.0, 0.0, 1.0), 90.0);
    const castle::math::matrix<double, 4U, 4U> xyz_rotation =
        castle::math::rotation_xyz(0.0, 0.0, castle::math::degrees_to_radians(90.0));

    EXPECT_TRUE(castle::math::near_equal(axis_rotation, xyz_rotation, 1.0e-10));
    const castle::math::matrix<double, 3U, 3U> expected =
        castle::math::matrix<double, 3U, 3U>(
            0.0, -1.0, 0.0,
            1.0, 0.0, 0.0,
            0.0, 0.0, 1.0);
    EXPECT_TRUE(castle::math::near_equal(
        castle::math::rotation_axis_angle_3d(
            castle::math::vector<double, 3U>(0.0, 0.0, 1.0),
            castle::math::degrees_to_radians(90.0)),
        expected,
        1.0e-10));
}

TEST(Transform, TwoAndThreeDimensionalCompositionOrder)
{
    const castle::math::matrix<float, 3U, 3U> composed_2d =
        castle::math::compose_transform_2d(
            castle::math::vector<float, 2U>(10.0F, 20.0F),
            castle::math::degrees_to_radians(90.0F),
            castle::math::vector<float, 2U>(2.0F, 3.0F));

    const castle::math::vector<float, 2U> composed_2d_expected(10.0F, 22.0F);
    EXPECT_EQ(
        castle::math::transform_point_2d(
            composed_2d,
            castle::math::vector<float, 2U>(1.0F, 0.0F)),
        composed_2d_expected);

    const castle::math::matrix<double, 4U, 4U> composed_3d =
        castle::math::compose_transform_3d(
            castle::math::vector<double, 3U>(10.0, 20.0, 30.0),
            0.0,
            0.0,
            castle::math::degrees_to_radians(90.0),
            castle::math::vector<double, 3U>(2.0, 2.0, 2.0));

    const castle::math::vector<double, 3U> result =
        castle::math::transform_point_3d(
            composed_3d,
            castle::math::vector<double, 3U>(1.0, 0.0, 0.0));
    EXPECT_NEAR(result[0U], 10.0, 1.0e-10);
    EXPECT_NEAR(result[1U], 22.0, 1.0e-10);
    EXPECT_NEAR(result[2U], 30.0, 1.0e-10);
}

TEST(Transform, GeometryTypeOverloads)
{
    const castle::math::point2d<float> point2(1.0F, 2.0F);
    const castle::math::vector2d<float> vector2(1.0F, 0.0F);
    const castle::math::point3d<float> point3(1.0F, 2.0F, 3.0F);
    const castle::math::vector3d<float> vector3(1.0F, 0.0F, 0.0F);

    const castle::math::point2d<float> point2_result =
        castle::math::transform_point_2d(
            castle::math::translation_2d(3.0F, 4.0F), point2);
    const castle::math::vector2d<float> vector2_result =
        castle::math::transform_vector_2d(
            castle::math::rotation_2d_degrees(90.0F), vector2);
    const castle::math::point3d<float> point3_result =
        castle::math::transform_point_3d(
            castle::math::translation_3d(3.0F, 4.0F, 5.0F), point3);
    const castle::math::vector3d<float> vector3_result =
        castle::math::transform_vector_3d(
            castle::math::rotation_z_degrees(90.0F), vector3);

    EXPECT_EQ(point2_result.x(), 4.0F);
    EXPECT_EQ(point2_result.y(), 6.0F);
    EXPECT_NEAR(vector2_result.x(), 0.0F, 1.0e-5F);
    EXPECT_NEAR(vector2_result.y(), 1.0F, 1.0e-5F);
    EXPECT_EQ(point3_result.x(), 4.0F);
    EXPECT_EQ(point3_result.y(), 6.0F);
    EXPECT_EQ(point3_result.z(), 8.0F);
    EXPECT_NEAR(vector3_result.x(), 0.0F, 1.0e-5F);
    EXPECT_NEAR(vector3_result.y(), 1.0F, 1.0e-5F);
    EXPECT_NEAR(vector3_result.z(), 0.0F, 1.0e-5F);
}
