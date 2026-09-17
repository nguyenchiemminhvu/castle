#include "sample_support.hpp"

#include "castle/math/isqrt.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::isqrt(0U) == 0U);
    CASTLE_SAMPLE_CHECK(castle::math::isqrt(1U) == 1U);
    CASTLE_SAMPLE_CHECK(castle::math::isqrt(15U) == 3U);
    CASTLE_SAMPLE_CHECK(castle::math::isqrt(16U) == 4U);
    CASTLE_SAMPLE_CHECK(castle::math::isqrt(81) == 9);
    return 0;
}
