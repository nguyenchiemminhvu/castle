#include "sample_support.hpp"

#include "castle/bit/bit_count.hpp"

#include <stdint.h>

int main()
{
    const uint64_t wide = 0xF0F000000000000FULL;
    const uint8_t sample = 0x30U;
    const uint8_t zero = 0U;

    CASTLE_SAMPLE_CHECK(castle::bit::popcount(wide) == 12U);
    CASTLE_SAMPLE_CHECK(castle::bit::popcount(sample) == 2U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_ones(sample) == 2U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_zeros(sample) == 6U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_leading_zeros(sample) == 2U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_trailing_zeros(sample) == 4U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_leading_zeros(zero) == 8U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_trailing_zeros(zero) == 8U);
    CASTLE_SAMPLE_CHECK(castle::bit::bit_width(sample) == 6U);
    CASTLE_SAMPLE_CHECK(castle::bit::bit_width(zero) == 0U);
    CASTLE_SAMPLE_CHECK(castle::bit::log2_floor(sample) == 5U);
    CASTLE_SAMPLE_CHECK(castle::bit::log2_floor(zero) == 0U);
    CASTLE_SAMPLE_CHECK(castle::bit::parity(sample) == 0U);
    CASTLE_SAMPLE_CHECK(castle::bit::parity<7U>() == 1U);

    return 0;
}
