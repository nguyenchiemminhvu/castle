#include <gtest/gtest.h>

#include "castle/math/linalg/quaternion.h"

TEST(Quaternion, IdentityConstructionAndLength)
{
    const castle::math::quaternion<float> identity;
    const castle::math::quaternion<float> explicit_identity(1.0F, 0.0F, 0.0F, 0.0F);

    EXPECT_FLOAT_EQ(identity.w(), 1.0F);
    EXPECT_FLOAT_EQ(identity.x(), 0.0F);
    EXPECT_FLOAT_EQ(identity.y(), 0.0F);
    EXPECT_FLOAT_EQ(identity.z(), 0.0F);
    EXPECT_FLOAT_EQ(identity.squared_length(), 1.0F);
    EXPECT_FLOAT_EQ(identity.length(), 1.0F);
    EXPECT_EQ(identity, explicit_identity);
}

TEST(Quaternion, ConjugateInverseAndNormalization)
{
    const castle::math::quaternion<double> value(2.0, 1.0, -2.0, 3.0);

    const castle::math::quaternion<double> conjugate = value.conjugate();
    EXPECT_DOUBLE_EQ(conjugate.w(), 2.0);
    EXPECT_DOUBLE_EQ(conjugate.x(), -1.0);
    EXPECT_DOUBLE_EQ(conjugate.y(), 2.0);
    EXPECT_DOUBLE_EQ(conjugate.z(), -3.0);

    const castle::math::quaternion<double> normalized = value.normalized();
    EXPECT_NEAR(normalized.length(), 1.0, 1.0e-12);

    const castle::math::quaternion<double> inverse = value.inverse();
    const castle::math::quaternion<double> product = value * inverse;
    EXPECT_NEAR(product.w(), 1.0, 1.0e-12);
    EXPECT_NEAR(product.x(), 0.0, 1.0e-12);
    EXPECT_NEAR(product.y(), 0.0, 1.0e-12);
    EXPECT_NEAR(product.z(), 0.0, 1.0e-12);
}

TEST(Quaternion, AxisAngleRotation)
{
    const castle::math::vector<float, 3U> axis(0.0F, 0.0F, 1.0F);
    const castle::math::quaternion<float> rotation =
        castle::math::quaternion<float>::from_axis_angle_degrees(axis, 90.0F);

    const castle::math::vector<float, 3U> rotated =
        rotation.rotate(castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));

    EXPECT_NEAR(rotated[0U], 0.0F, 1.0e-5F);
    EXPECT_NEAR(rotated[1U], 1.0F, 1.0e-5F);
    EXPECT_NEAR(rotated[2U], 0.0F, 1.0e-5F);
}

TEST(Quaternion, CompositionAndRotationMatrix)
{
    const castle::math::quaternion<double> x_rotation =
        castle::math::quaternion<double>::from_axis_angle_degrees(
            castle::math::vector<double, 3U>(1.0, 0.0, 0.0), 90.0);
    const castle::math::quaternion<double> z_rotation =
        castle::math::quaternion<double>::from_axis_angle_degrees(
            castle::math::vector<double, 3U>(0.0, 0.0, 1.0), 90.0);

    const castle::math::quaternion<double> composed = z_rotation * x_rotation;
    const castle::math::matrix<double, 3U, 3U> matrix = composed.to_matrix();
    const castle::math::matrix<double, 3U, 3U> orthonormal = matrix * matrix.transpose();

    EXPECT_TRUE(castle::math::near_equal(
        orthonormal,
        castle::math::identity<double, 3U>(),
        1.0e-10));
    EXPECT_NEAR(castle::math::determinant(matrix), 1.0, 1.0e-10);

    const castle::math::vector<double, 3U> rotated =
        composed.rotate(castle::math::vector<double, 3U>(1.0, 0.0, 0.0));
    EXPECT_NEAR(rotated[0U], 0.0, 1.0e-10);
    EXPECT_NEAR(rotated[1U], 1.0, 1.0e-10);
    EXPECT_NEAR(rotated[2U], 0.0, 1.0e-10);
}

TEST(Quaternion, EulerConstruction)
{
    const castle::math::quaternion<double> value =
        castle::math::quaternion<double>::from_euler_xyz(0.2, -0.4, 0.7);

    EXPECT_NEAR(value.length(), 1.0, 1.0e-12);
    EXPECT_TRUE(castle::math::near_equal(
        value.to_matrix() * value.to_matrix().transpose(),
        castle::math::identity<double, 3U>(),
        1.0e-10));
}
