#include "sample_support.hpp"

#include "castle/math/linalg/matrix.hpp"

int main()
{
    const float epsilon = 1.0e-5F;

    const castle::math::matrix<float, 2U, 2U> value(1.0F, 2.0F, 3.0F, 4.0F);
    const castle::math::vector<float, 2U> first_row = value.row(0U);
    const castle::math::vector<float, 2U> second_column = value.column(1U);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(first_row, castle::math::vector<float, 2U>(1.0F, 2.0F), epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(second_column, castle::math::vector<float, 2U>(2.0F, 4.0F), epsilon));

    const castle::math::vector<float, 2U> product =
        value * castle::math::vector<float, 2U>(1.0F, 2.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(product[0U], 5.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(product[1U], 11.0F, epsilon));

    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::determinant(value), -2.0F, epsilon));

    const castle::math::matrix<float, 2U, 2U> inverse_value =
        castle::math::inverse(value);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(inverse_value(0U, 0U), -2.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(inverse_value(0U, 1U), 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(inverse_value(1U, 0U), 1.5F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(inverse_value(1U, 1U), -0.5F, epsilon));

    const castle::math::matrix<float, 2U, 2U> hadamard_value =
        castle::math::hadamard(value, castle::math::matrix<float, 2U, 2U>(2.0F, 0.5F, 1.0F, 2.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(hadamard_value(0U, 0U), 2.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(hadamard_value(0U, 1U), 1.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(hadamard_value(1U, 0U), 3.0F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(hadamard_value(1U, 1U), 8.0F, epsilon));

    return 0;
}
