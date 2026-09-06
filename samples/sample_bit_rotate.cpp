#include "sample_support.h"

#include "castle/bit/bit_rotate.h"

// Scenario: implementing a compact CRC/hash-like mixing stage without lookup tables.
#include <stdint.h>

int main()
{
    const uint32_t word = 0x12345678U;
    CASTLE_SAMPLE_CHECK(castle::bit::rotate_left(word, 8U) == 0x34567812U);
    CASTLE_SAMPLE_CHECK(castle::bit::rotate_right(word, 8U) == 0x78123456U);
    return 0;
}
