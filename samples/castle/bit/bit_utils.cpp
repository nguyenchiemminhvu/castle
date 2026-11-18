#include "sample_support.hpp"

#include "castle/bit/bit_utils.hpp"

#include <stdint.h>

int main()
{
    const uint32_t sample = 0x000000B4U;

    CASTLE_SAMPLE_CHECK(castle::bit::extract_lowest_set_bit(sample) == 0x00000004U);
    CASTLE_SAMPLE_CHECK(castle::bit::extract_lowest_set_bit(0U) == 0U);
    CASTLE_SAMPLE_CHECK(castle::bit::extract_highest_set_bit(sample) == 0x00000080U);
    CASTLE_SAMPLE_CHECK(castle::bit::extract_highest_set_bit(0U) == 0U);

    CASTLE_SAMPLE_CHECK(castle::bit::extract_field(static_cast<uint16_t>(0xA5B6U), 4U, 4U) == static_cast<uint16_t>(0x000BU));
    CASTLE_SAMPLE_CHECK((castle::bit::extract_field<8U, 4U>(static_cast<uint16_t>(0xA5B6U)) == static_cast<uint16_t>(0x0005U)));
    CASTLE_SAMPLE_CHECK(castle::bit::extract_field(static_cast<uint16_t>(0xA5B6U), 16U, 4U) == static_cast<uint16_t>(0x0000U));
    CASTLE_SAMPLE_CHECK(castle::bit::extract_field(static_cast<uint16_t>(0xA5B6U), 4U, 0U) == static_cast<uint16_t>(0x0000U));

    CASTLE_SAMPLE_CHECK(castle::bit::insert_field(static_cast<uint16_t>(0xFF00U), static_cast<uint16_t>(0x0005U), 4U, 4U) == static_cast<uint16_t>(0xFF50U));
    CASTLE_SAMPLE_CHECK((castle::bit::insert_field<8U, 4U>(static_cast<uint16_t>(0x0000U), static_cast<uint16_t>(0x000AU)) == static_cast<uint16_t>(0x0A00U)));
    CASTLE_SAMPLE_CHECK(castle::bit::insert_field(static_cast<uint16_t>(0x1234U), static_cast<uint16_t>(0xFFFFU), 0U, 4U) == static_cast<uint16_t>(0x123FU));
    CASTLE_SAMPLE_CHECK(castle::bit::insert_field(static_cast<uint16_t>(0x1234U), static_cast<uint16_t>(0x0001U), 16U, 4U) == static_cast<uint16_t>(0x1234U));

    return 0;
}
