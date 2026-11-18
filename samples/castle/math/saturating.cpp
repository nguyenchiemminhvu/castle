#include "sample_support.hpp"

#include "castle/math/saturating.hpp"

#include <stdint.h>

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::saturating_add<uint8_t>(250U, 20U) == UINT8_MAX);
    CASTLE_SAMPLE_CHECK(castle::math::saturating_sub<uint8_t>(5U, 8U) == 0U);
    CASTLE_SAMPLE_CHECK(castle::math::saturating_add<int8_t>(120, 20) == INT8_MAX);
    CASTLE_SAMPLE_CHECK(castle::math::saturating_add<int8_t>(-120, -20) == INT8_MIN);
    CASTLE_SAMPLE_CHECK(castle::math::saturating_sub<int8_t>(-120, 20) == INT8_MIN);
    CASTLE_SAMPLE_CHECK(castle::math::saturating_sub<int8_t>(120, -20) == INT8_MAX);
    return 0;
}
