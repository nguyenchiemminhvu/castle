#include <gtest/gtest.h>

#include "castle/math/linalg/matrix.h"

TEST(Matrix, ConstructionAccessAndIteration)
{
    const castle::math::matrix<int, 2U, 3U> value(1, 2, 3, 4, 5, 6);

    EXPECT_EQ(value.rows(), 2U);
    EXPECT_EQ(value.columns(), 3U);
    EXPECT_EQ(value.size(), 6U);
    EXPECT_EQ(value(1U, 2U), 6);
    EXPECT_EQ(value.at(0U, 1U), 2);
    EXPECT_EQ(value.data()[4U], 5);
    EXPECT_EQ(value.end() - value.begin(), 6);
}

TEST(Matrix, ArithmeticAndScalarOperations)
{
    const castle::math::matrix<int, 2U, 2U> first(1, 2, 3, 4);
    const castle::math::matrix<int, 2U, 2U> second(5, 6, 7, 8);

    const castle::math::matrix<int, 2U, 2U> add_expected(6, 8, 10, 12);
    const castle::math::matrix<int, 2U, 2U> subtract_expected(4, 4, 4, 4);
    const castle::math::matrix<int, 2U, 2U> negative_expected(-1, -2, -3, -4);
    const castle::math::matrix<int, 2U, 2U> multiply_expected(2, 4, 6, 8);
    const castle::math::matrix<int, 2U, 2U> divide_expected(0, 1, 1, 2);

    EXPECT_EQ(first + second, add_expected);
    EXPECT_EQ(second - first, subtract_expected);
    EXPECT_EQ(-first, negative_expected);
    EXPECT_EQ(first * 2, multiply_expected);
    EXPECT_EQ(2 * first, multiply_expected);
    EXPECT_EQ(first / 2, divide_expected);

    castle::math::matrix<int, 2U, 2U> value = first;
    value += second;
    EXPECT_EQ(value, add_expected);
    value -= second;
    EXPECT_EQ(value, first);
    value *= 3;
    const castle::math::matrix<int, 2U, 2U> compound_expected(3, 6, 9, 12);
    EXPECT_EQ(value, compound_expected);
    value /= 3;
    EXPECT_EQ(value, first);
}

TEST(Matrix, RowsColumnsAndTranspose)
{
    const castle::math::matrix<int, 2U, 3U> value(1, 2, 3, 4, 5, 6);

    const castle::math::vector<int, 3U> first_row(1, 2, 3);
    const castle::math::vector<int, 3U> second_row(4, 5, 6);
    const castle::math::vector<int, 2U> first_column(1, 4);
    const castle::math::vector<int, 2U> last_column(3, 6);

    EXPECT_EQ(value.row(0U), first_row);
    EXPECT_EQ(value.row(1U), second_row);
    EXPECT_EQ(value.column(0U), first_column);
    EXPECT_EQ(value.column(2U), last_column);

    const castle::math::matrix<int, 3U, 2U> transposed = value.transpose();
    const castle::math::matrix<int, 3U, 2U> transpose_expected(1, 4, 2, 5, 3, 6);
    EXPECT_EQ(transposed, transpose_expected);
    EXPECT_EQ(castle::math::transpose(value), transposed);
}

TEST(Matrix, MatrixAndVectorProducts)
{
    const castle::math::matrix<int, 2U, 3U> first(1, 2, 3, 4, 5, 6);
    const castle::math::matrix<float, 3U, 2U> second(1.0F, 2.0F, 0.0F, 1.0F, 2.0F, 0.0F);

    const castle::math::matrix<float, 2U, 2U> product = first * second;
    EXPECT_FLOAT_EQ(product(0U, 0U), 7.0F);
    EXPECT_FLOAT_EQ(product(0U, 1U), 4.0F);
    EXPECT_FLOAT_EQ(product(1U, 0U), 16.0F);
    EXPECT_FLOAT_EQ(product(1U, 1U), 13.0F);

    const castle::math::vector<double, 3U> input(1.0, 2.0, 3.0);
    const castle::math::vector<double, 2U> output = first * input;
    EXPECT_DOUBLE_EQ(output[0U], 14.0);
    EXPECT_DOUBLE_EQ(output[1U], 32.0);
}

TEST(Matrix, ProductsAndNorms)
{
    const castle::math::matrix<int, 2U, 2U> first(1, 2, 3, 4);
    const castle::math::matrix<int, 2U, 2U> second(5, 6, 7, 8);

    const castle::math::matrix<int, 2U, 2U> hadamard_expected(5, 12, 21, 32);
    EXPECT_EQ(castle::math::hadamard(first, second), hadamard_expected);
    EXPECT_EQ(castle::math::frobenius_squared(first), 30);

    const castle::math::matrix<int, 3U, 3U> diagonal_expected(
        1, 0, 0,
        0, 2, 0,
        0, 0, 3);
    EXPECT_EQ(
        castle::math::diagonal_matrix(castle::math::vector<int, 3U>(1, 2, 3)),
        diagonal_expected);

    const castle::math::matrix<int, 3U, 3U> outer_expected(
        1, 2, 3,
        2, 4, 6,
        3, 6, 9);
    EXPECT_EQ(
        castle::math::outer_product(
            castle::math::vector<int, 3U>(1, 2, 3),
            castle::math::vector<int, 3U>(1, 2, 3)),
        outer_expected);

    const castle::math::matrix<int, 3U, 3U> skew_expected(
        0, -3, 2,
        3, 0, -1,
        -2, 1, 0);
    EXPECT_EQ(
        castle::math::skew_symmetric(castle::math::vector<int, 3U>(1, 2, 3)),
        skew_expected);
}

TEST(Matrix, DeterminantTraceInverseAndIdentity)
{
    const castle::math::matrix<float, 3U, 3U> value(
        1.0F, 2.0F, 3.0F,
        0.0F, 1.0F, 4.0F,
        5.0F, 6.0F, 0.0F);

    EXPECT_NEAR(castle::math::determinant(value), 1.0F, 1.0e-6F);
    EXPECT_FLOAT_EQ(value.trace(), 2.0F);
    EXPECT_NEAR(castle::math::frobenius_squared(value), 92.0F, 1.0e-6F);

    const castle::math::matrix<float, 3U, 3U> inverse = castle::math::inverse(value);
    EXPECT_TRUE(castle::math::near_equal(
        value * inverse,
        castle::math::identity<float, 3U>(),
        1.0e-5F));
}

TEST(Matrix, SingularMatrixIsRejected)
{
    const castle::math::matrix<double, 2U, 2U> singular(1.0, 2.0, 2.0, 4.0);
    castle::math::matrix<double, 2U, 2U> inverse;

    EXPECT_FALSE(castle::math::try_inverse(singular, inverse));
}

TEST(Matrix, NearEqualAndAliases)
{
    const castle::math::matrix<double, 2U, 2U> first(1.0, 2.0, 3.0, 4.0);
    const castle::math::matrix<double, 2U, 2U> second(1.0 + 1.0e-10, 2.0, 3.0, 4.0);

    EXPECT_TRUE(castle::math::near_equal(first, second, 1.0e-9));
    EXPECT_FALSE(castle::math::near_equal(first, second, 1.0e-12));

    const castle::math::matrix2x2<int> matrix2(1, 0, 0, 1);
    const castle::math::matrix3x3<int> matrix3 = castle::math::identity<int, 3U>();
    const castle::math::matrix4x4<int> matrix4 = castle::math::identity<int, 4U>();
    EXPECT_EQ(matrix2(0U, 0U), 1);
    EXPECT_EQ(matrix3(2U, 2U), 1);
    EXPECT_EQ(matrix4(3U, 3U), 1);
}
