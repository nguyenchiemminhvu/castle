#include "sample_support.h"

#include "castle/bit/bit_utils.h"

// Scenario: decoding the bit fields of a sampled peripheral status register.
#include <stdint.h>

int main()
{
    const uint32_t status = 0x00A0U;
    CASTLE_SAMPLE_CHECK(castle::bit::extract_lowest_set_bit(status) == 0x20U);
    CASTLE_SAMPLE_CHECK(castle::bit::extract_highest_set_bit(status) == 0x80U);
    CASTLE_SAMPLE_CHECK(castle::bit::extract_field(0xA5B6U, 4U, 4U) == 0x0BU);
    return 0;
}
