#include "sample_support.hpp"

#include "castle/math/round.hpp"

#include <stdint.h>

int main()
{
    // Quantise 1.23456 m to a 0.0001 m fixed-point field.
    CASTLE_SAMPLE_CHECK(castle::math::round_half_away<int64_t>(1.23456 / 0.0001) == 12346);

    // Ties round away from zero on both sides.
    CASTLE_SAMPLE_CHECK(castle::math::round_half_away<int32_t>(2.5) == 3);
    CASTLE_SAMPLE_CHECK(castle::math::round_half_away<int32_t>(-2.5) == -3);
    CASTLE_SAMPLE_CHECK(castle::math::round_half_away<uint8_t>(41.6f) == 42U);
    return 0;
}
