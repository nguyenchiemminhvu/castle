#include "sample_support.hpp"

#include "castle/bit/bit.hpp"

#include <stdint.h>

int main()
{
    uint8_t value = 0U;
    value = castle::bit::set(value, 3U);

    CASTLE_SAMPLE_CHECK(castle::bit::test(value, 3U));
    CASTLE_SAMPLE_CHECK(castle::bit::count_ones(value) == 1U);
    CASTLE_SAMPLE_CHECK(castle::bit::single_bit_mask<uint8_t>(3U) == static_cast<uint8_t>(0x08U));
    CASTLE_SAMPLE_CHECK(castle::bit::is_power_of_two(static_cast<uint8_t>(8U)));
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(static_cast<uint8_t>(0x0DU)) == static_cast<uint8_t>(0xB0U));
    CASTLE_SAMPLE_CHECK(castle::bit::rotate_left(static_cast<uint8_t>(0x12U), 4U) == static_cast<uint8_t>(0x21U));
    CASTLE_SAMPLE_CHECK(castle::bit::extract_field(static_cast<uint16_t>(0x00F0U), 4U, 4U) == static_cast<uint16_t>(0x000FU));

    castle::bit::flags<uint8_t, 0x0FU> status(0x12U);
    CASTLE_SAMPLE_CHECK(status.value() == static_cast<uint8_t>(0x02U));

    return 0;
}
