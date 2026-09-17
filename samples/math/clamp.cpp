#include "sample_support.hpp"

#include "castle/math/clamp.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::clamp(150, 0, 100) == 100);
    CASTLE_SAMPLE_CHECK(castle::math::clamp(-1, 0, 100) == 0);
    CASTLE_SAMPLE_CHECK(castle::math::clamp(42, 0, 100) == 42);
    CASTLE_SAMPLE_CHECK(castle::math::clamp(-0.5f, 0.0f, 1.0f) == 0.0f);
    return 0;
}
