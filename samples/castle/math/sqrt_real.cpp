#include "sample_support.hpp"

#include "castle/math/sqrt_real.hpp"

int main()
{
    const float root_a = castle::math::sqrt_real(2.25f);
    const double root_b = castle::math::sqrt_real(2.0);

    CASTLE_SAMPLE_CHECK(castle::math::sqrt_real(0.0f) == 0.0f);
    CASTLE_SAMPLE_CHECK(root_a > 1.49f && root_a < 1.51f);
    CASTLE_SAMPLE_CHECK(root_b > 1.4141 && root_b < 1.4143);
    return 0;
}
