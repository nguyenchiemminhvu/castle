#include "sample_support.hpp"

#include "castle/math/lerp.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 0.0f) == 10.0f);
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 0.5f) == 15.0f);
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 1.0f) == 20.0f);
    CASTLE_SAMPLE_CHECK(castle::math::lerp(10.0f, 20.0f, 1.5f) == 25.0f);
    return 0;
}
