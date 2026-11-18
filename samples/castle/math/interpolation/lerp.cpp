#include "sample_support.hpp"

#include "castle/math/interpolation/lerp.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 0.0f) == 10.0f);
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 0.5f) == 15.0f);
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 1.0f) == 20.0f);
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 1.5f) == 25.0f);

    const float xs[] = {0.0F, 1.0F, 2.0F};
    const float ys[] = {0.0F, 2.0F, 8.0F};
    CASTLE_SAMPLE_CHECK(castle::math::linear_interpolate(xs, ys, 3U, 1.5F) == 5.0F);
    return 0;
}
