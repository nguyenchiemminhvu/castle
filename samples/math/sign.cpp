#include "sample_support.hpp"

#include "castle/math/sign.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::sign(-12) == -1);
    CASTLE_SAMPLE_CHECK(castle::math::sign(0) == 0);
    CASTLE_SAMPLE_CHECK(castle::math::sign(3.5f) == 1);
    CASTLE_SAMPLE_CHECK(castle::math::sign(0U) == 0);
    CASTLE_SAMPLE_CHECK(castle::math::sign(7U) == 1);
    return 0;
}
