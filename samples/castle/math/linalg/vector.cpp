#include "sample_support.hpp"

#include "castle/math/linalg/vector.hpp"

int main()
{
    const float epsilon = 1.0e-5F;

    const castle::math::vector<float, 3U> a(1.0F, 2.0F, 3.0F);
    const castle::math::vector<float, 3U> b(4.0F, 5.0F, 6.0F);
    const castle::math::vector<float, 3U> sum = a + b;
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(sum, castle::math::vector<float, 3U>(5.0F, 7.0F, 9.0F), epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(a.dot(b), 32.0F, epsilon));

    const castle::math::vector<float, 2U> value(3.0F, 4.0F);
    const castle::math::vector<float, 2U> unit = value.normalized();
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(unit[0U], 0.6F, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(unit[1U], 0.8F, epsilon));

    const castle::math::vector<float, 2U> projection =
        castle::math::project(
            castle::math::vector<float, 2U>(3.0F, 4.0F),
            castle::math::vector<float, 2U>(1.0F, 0.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(projection, castle::math::vector<float, 2U>(3.0F, 0.0F), epsilon));

    const castle::math::vector<float, 2U> reflection =
        castle::math::reflect(
            castle::math::vector<float, 2U>(1.0F, -1.0F),
            castle::math::vector<float, 2U>(0.0F, 1.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(reflection, castle::math::vector<float, 2U>(1.0F, 1.0F), epsilon));

    const castle::math::vector<float, 3U> cross_value =
        castle::math::cross(
            castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F),
            castle::math::vector<float, 3U>(0.0F, 1.0F, 0.0F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(cross_value, castle::math::vector<float, 3U>(0.0F, 0.0F, 1.0F), epsilon));

    return 0;
}
