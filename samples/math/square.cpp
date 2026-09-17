#include "sample_support.hpp"

#include "castle/math/square.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::square(-7) == 49);
    CASTLE_SAMPLE_CHECK(castle::math::square(12U) == 144U);
    CASTLE_SAMPLE_CHECK(castle::math::square(1.5f) > 2.24f && castle::math::square(1.5f) < 2.26f);
    return 0;
}
