#include "sample_support.h"

#include "castle/bit/bit_math.h"

// Scenario: checking DMA ring sizes, divisibility and power-of-two hardware-buffer constraints.
#include <stdint.h>

int main()
{
    CASTLE_SAMPLE_CHECK(castle::bit::is_even(100U));
    CASTLE_SAMPLE_CHECK(castle::bit::is_odd(101U));
    CASTLE_SAMPLE_CHECK(castle::bit::is_power_of_two(256U));
    CASTLE_SAMPLE_CHECK(!castle::bit::is_power_of_two(0U));
    static_assert(castle::bit::is_even_const<64U>::value, "compile-time parity");
    static_assert(castle::bit::is_power_of_two_const<128U>::value, "compile-time power-of-two");
    return 0;
}
