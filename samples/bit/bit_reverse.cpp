#include "sample_support.hpp"

#include "castle/bit/bit_reverse.hpp"

#include <stdint.h>

int main()
{
    const int16_t signed_value = static_cast<int16_t>(0x0003U);

    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(static_cast<uint8_t>(0xB0U)) == static_cast<uint8_t>(0x0DU));
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(static_cast<uint16_t>(0x00F0U)) == static_cast<uint16_t>(0x0F00U));
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(0x00000001U) == 0x80000000U);
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(0x0000000000000001ULL) == 0x8000000000000000ULL);
    CASTLE_SAMPLE_CHECK(static_cast<uint16_t>(castle::bit::reverse_bits(signed_value)) == static_cast<uint16_t>(0xC000U));

    CASTLE_SAMPLE_CHECK(castle::bit::byte_swap(static_cast<uint8_t>(0xABU)) == static_cast<uint8_t>(0xABU));
    CASTLE_SAMPLE_CHECK(castle::bit::byte_swap(static_cast<uint16_t>(0x1234U)) == static_cast<uint16_t>(0x3412U));
    CASTLE_SAMPLE_CHECK(castle::bit::byte_swap(0x12345678U) == 0x78563412U);
    CASTLE_SAMPLE_CHECK(castle::bit::byte_swap(0x0102030405060708ULL) == 0x0807060504030201ULL);
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bytes(static_cast<uint16_t>(0x00F1U)) == static_cast<uint16_t>(0xF100U));

    return 0;
}
