#include <gtest/gtest.h>

#include "castle/math/linalg/vector.h"

TEST(Vector, ConstructionAndElementAccess)
{
    const castle::math::vector<int, 3U> value(1, 2, 3);

    EXPECT_EQ(value.size(), 3U);
    EXPECT_EQ(value[0U], 1);
    EXPECT_EQ(value[1U], 2);
    EXPECT_EQ(value[2U], 3);
    EXPECT_EQ(value.at(2U), 3);
    EXPECT_EQ(value.data()[1U], 2);
    EXPECT_EQ(value.end() - value.begin(), 3);
}

TEST(Vector, Arithmetic)
{
    const castle::math::vector<int, 3U> first(1, 2, 3);
    const castle::math::vector<int, 3U> second(4, 5, 6);

    const castle::math::vector<int, 3U> add_expected(5, 7, 9);
    const castle::math::vector<int, 3U> subtract_expected(3, 3, 3);
    const castle::math::vector<int, 3U> negative_expected(-1, -2, -3);
    const castle::math::vector<int, 3U> multiply_expected(2, 4, 6);
    const castle::math::vector<int, 3U> divide_expected(0, 1, 1);
    const castle::math::vector<int, 3U> hadamard_expected(4, 10, 18);

    EXPECT_EQ(first + second, add_expected);
    EXPECT_EQ(second - first, subtract_expected);
    EXPECT_EQ(-first, negative_expected);
    EXPECT_EQ(first * 2, multiply_expected);
    EXPECT_EQ(2 * first, multiply_expected);
    EXPECT_EQ(first / 2, divide_expected);
    EXPECT_EQ(first.hadamard(second), hadamard_expected);

    castle::math::vector<int, 3U> value = first;
    value += second;
    EXPECT_EQ(value, add_expected);
    value -= second;
    EXPECT_EQ(value, first);
    value *= 3;
    const castle::math::vector<int, 3U> compound_expected(3, 6, 9);
    EXPECT_EQ(value, compound_expected);
    value /= 3;
    EXPECT_EQ(value, first);
}

TEST(Vector, ProductsAndComponents)
{
    const castle::math::vector<float, 3U> first(1.0F, 2.0F, 3.0F);
    const castle::math::vector<float, 3U> second(4.0F, -5.0F, 6.0F);

    EXPECT_FLOAT_EQ(first.dot(second), 12.0F);
    EXPECT_FLOAT_EQ(castle::math::dot(first, second), 12.0F);
    const castle::math::vector<float, 3U> min_expected(1.0F, -5.0F, 3.0F);
    const castle::math::vector<float, 3U> max_expected(4.0F, 2.0F, 6.0F);
    const castle::math::vector<float, 3U> absolute_input(-1.0F, 2.0F, -3.0F);
    const castle::math::vector<float, 3U> absolute_expected(1.0F, 2.0F, 3.0F);

    EXPECT_EQ(first.component_min(second), min_expected);
    EXPECT_EQ(first.component_max(second), max_expected);
    EXPECT_EQ(absolute_input.abs(), absolute_expected);

    const castle::math::vector<float, 3U> cross_expected(27.0F, 6.0F, -13.0F);
    EXPECT_EQ(castle::math::cross(first, second), cross_expected);
    EXPECT_FLOAT_EQ(
        castle::math::scalar_triple_product(
            first,
            second,
            castle::math::vector<float, 3U>(1.0F, 0.0F, 1.0F)),
        14.0F);
}

TEST(Vector, LengthNormalizationDistanceAndLerp)
{
    const castle::math::vector<float, 2U> first(3.0F, 4.0F);
    const castle::math::vector<float, 2U> second(6.0F, 8.0F);

    EXPECT_FLOAT_EQ(first.squared_length(), 25.0F);
    EXPECT_FLOAT_EQ(first.length(), 5.0F);
    EXPECT_NEAR(first.normalized()[0U], 0.6F, 1.0e-6F);
    EXPECT_NEAR(first.normalized()[1U], 0.8F, 1.0e-6F);
    EXPECT_FLOAT_EQ(first.distance_to(second), 5.0F);
    const castle::math::vector<float, 2U> lerp_expected(4.5F, 6.0F);
    EXPECT_EQ(first.lerp_to(second, 0.5F), lerp_expected);
}

TEST(Vector, ProjectionReflectionAndNearEqual)
{
    const castle::math::vector<float, 2U> value(3.0F, 4.0F);
    const castle::math::vector<float, 2U> basis(1.0F, 0.0F);

    const castle::math::vector<float, 2U> projection_expected(3.0F, 0.0F);
    const castle::math::vector<float, 2U> reflection_expected(-3.0F, 4.0F);
    EXPECT_EQ(castle::math::project(value, basis), projection_expected);
    EXPECT_EQ(castle::math::reflect(value, basis), reflection_expected);
    EXPECT_TRUE(castle::math::near_equal(
        value,
        castle::math::vector<float, 2U>(3.0F + 1.0e-6F, 4.0F),
        1.0e-5F));
    EXPECT_FALSE(castle::math::near_equal(
        value,
        castle::math::vector<float, 2U>(3.0F + 1.0e-3F, 4.0F),
        1.0e-5F));
}

TEST(Vector, AliasesAndConstIteration)
{
    const castle::math::vector2<float> value(1.0F, 2.0F);
    const castle::math::vector3<float> other(1.0F, 2.0F, 3.0F);
    const castle::math::vector4<float> last(1.0F, 2.0F, 3.0F, 4.0F);

    EXPECT_EQ(value.static_size, 2U);
    EXPECT_EQ(value.cend() - value.cbegin(), 2);
    EXPECT_EQ(other.size(), 3U);
    EXPECT_EQ(last.size(), 4U);
}
