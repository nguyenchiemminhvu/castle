#include "sample_support.hpp"

#include "castle/bit/bit_mask.hpp"

#include <stdint.h>

int main()
{
    static_assert(castle::bit::all_bits_mask<uint8_t>::value == 0xFFU, "all bits mask");
    static_assert(castle::bit::single_bit_mask_const<3U, uint8_t>::value == 0x08U, "single bit mask");
    static_assert(castle::bit::low_bits_mask_const<5U, uint8_t>::value == 0x1FU, "low bits mask");
    static_assert(castle::bit::high_bits_mask_const<3U, uint8_t>::value == 0xE0U, "high bits mask");
    static_assert(castle::bit::range_mask_const<2U, 3U, uint8_t>::value == 0x1CU, "range mask");

    CASTLE_SAMPLE_CHECK(castle::bit::single_bit_mask<uint16_t>(9U) == static_cast<uint16_t>(0x0200U));
    CASTLE_SAMPLE_CHECK(castle::bit::low_bits_mask<uint8_t>(0U) == static_cast<uint8_t>(0x00U));
    CASTLE_SAMPLE_CHECK(castle::bit::low_bits_mask<uint8_t>(5U) == static_cast<uint8_t>(0x1FU));
    CASTLE_SAMPLE_CHECK(castle::bit::low_bits_mask<uint8_t>(8U) == static_cast<uint8_t>(0xFFU));
    CASTLE_SAMPLE_CHECK(castle::bit::high_bits_mask<uint8_t>(0U) == static_cast<uint8_t>(0x00U));
    CASTLE_SAMPLE_CHECK(castle::bit::high_bits_mask<uint8_t>(3U) == static_cast<uint8_t>(0xE0U));
    CASTLE_SAMPLE_CHECK(castle::bit::high_bits_mask<uint8_t>(8U) == static_cast<uint8_t>(0xFFU));
    CASTLE_SAMPLE_CHECK(castle::bit::range_mask<uint8_t>(2U, 3U) == static_cast<uint8_t>(0x1CU));

    return 0;
}
