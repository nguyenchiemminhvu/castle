#include "sample_support.hpp"

#include "castle/bit/bit_math.hpp"

#include <stdint.h>

int main()
{
    CASTLE_SAMPLE_CHECK(castle::bit::is_even(100U));
    CASTLE_SAMPLE_CHECK(!castle::bit::is_even(101U));
    CASTLE_SAMPLE_CHECK(castle::bit::is_odd(101U));
    CASTLE_SAMPLE_CHECK(!castle::bit::is_odd(100U));
    CASTLE_SAMPLE_CHECK(castle::bit::is_even_const<64U>::value);
    CASTLE_SAMPLE_CHECK(castle::bit::is_odd_const<65U>::value);

    CASTLE_SAMPLE_CHECK(castle::bit::is_power_of_two(256U));
    CASTLE_SAMPLE_CHECK(!castle::bit::is_power_of_two(0U));
    CASTLE_SAMPLE_CHECK(!castle::bit::is_power_of_two(-8));
    CASTLE_SAMPLE_CHECK(castle::bit::is_power_of_two_const<128U>::value);

    CASTLE_SAMPLE_CHECK(castle::bit::next_power_of_two(static_cast<uint32_t>(0U)) == 1U);
    CASTLE_SAMPLE_CHECK(castle::bit::next_power_of_two(static_cast<uint32_t>(300U)) == 512U);
    CASTLE_SAMPLE_CHECK(castle::bit::next_power_of_two<300U>() == 512U);
    CASTLE_SAMPLE_CHECK(castle::bit::next_power_of_two_const<300U>::value == 512U);

    CASTLE_SAMPLE_CHECK(castle::bit::previous_power_of_two(static_cast<uint32_t>(0U)) == 0U);
    CASTLE_SAMPLE_CHECK(castle::bit::previous_power_of_two(static_cast<uint32_t>(300U)) == 256U);
    CASTLE_SAMPLE_CHECK(castle::bit::previous_power_of_two<300U>() == 256U);
    CASTLE_SAMPLE_CHECK(castle::bit::previous_power_of_two_const<300U>::value == 256U);

    CASTLE_SAMPLE_CHECK(castle::bit::align_up(static_cast<uint32_t>(37U), static_cast<uint32_t>(8U)) == 40U);
    CASTLE_SAMPLE_CHECK(castle::bit::align_down(static_cast<uint32_t>(37U), static_cast<uint32_t>(8U)) == 32U);
    CASTLE_SAMPLE_CHECK(castle::bit::is_aligned(static_cast<uint32_t>(40U), static_cast<uint32_t>(8U)));
    CASTLE_SAMPLE_CHECK(!castle::bit::is_aligned(static_cast<uint32_t>(37U), static_cast<uint32_t>(8U)));

    CASTLE_SAMPLE_CHECK(castle::bit::sign(-42) == -1);
    CASTLE_SAMPLE_CHECK(castle::bit::sign(0) == 0);
    CASTLE_SAMPLE_CHECK(castle::bit::sign(42U) == 1);
    CASTLE_SAMPLE_CHECK(castle::bit::sign<0U>() == 0);
    CASTLE_SAMPLE_CHECK(castle::bit::sign<7U>() == 1);

    return 0;
}
