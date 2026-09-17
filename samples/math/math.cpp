#include "sample_support.hpp"

#include "castle/math/math.hpp"

int main()
{
    static_assert(castle::sqrt<81U>::value == 9U, "");

    const float epsilon = castle::meta::floating_epsilon<float>::value;

    CASTLE_SAMPLE_CHECK(castle::math::clamp(150, 0, 100) == 100);
    CASTLE_SAMPLE_CHECK(castle::math::powi(3U, 3U) == 27U);
    CASTLE_SAMPLE_CHECK(castle::math::is_equal(1.0f, 1.0f + (epsilon * 0.5f)));
    CASTLE_SAMPLE_CHECK(castle::math::is_zero(epsilon * 0.5f));
    return 0;
}
