#include "sample_support.hpp"

#include "castle/bit/bit_rotate.hpp"

#include <stdint.h>

int main()
{
    const uint32_t word = 0x12345678U;
    const int8_t signed_word = static_cast<int8_t>(0x12U);

    CASTLE_SAMPLE_CHECK(castle::bit::rotate_left(word, 8U) == 0x34567812U);
    CASTLE_SAMPLE_CHECK(castle::bit::rotate_right(word, 8U) == 0x78123456U);
    CASTLE_SAMPLE_CHECK(castle::bit::rotate_left(word, 0U) == word);
    CASTLE_SAMPLE_CHECK(castle::bit::rotate_right(word, 32U) == word);
    CASTLE_SAMPLE_CHECK(castle::bit::rotate_left(word, 40U) == 0x34567812U);
    CASTLE_SAMPLE_CHECK(static_cast<uint8_t>(castle::bit::rotate_left(signed_word, 4U)) == static_cast<uint8_t>(0x21U));

    return 0;
}
