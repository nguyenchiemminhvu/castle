#include "sample_support.h"

#include "castle/bit/bit_reverse.h"

// Scenario: bit-order conversion for a wire protocol whose serial bit order differs from CPU order.
#include <stdint.h>

int main()
{
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(static_cast<uint8_t>(0xB0U)) == 0x0DU);
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(0x00000001U) == 0x80000000U);
    CASTLE_SAMPLE_CHECK(castle::bit::reverse_bits(0x0000000000000001ULL) == 0x8000000000000000ULL);
    return 0;
}
