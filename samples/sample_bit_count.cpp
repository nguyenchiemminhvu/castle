#include "sample_support.h"

#include "castle/bit/bit_count.h"

// Scenario: validating a packed sensor/status word before transmitting it.
#include <stdint.h>

int main()
{
    const uint32_t sample = 0x00F00010U;
    CASTLE_SAMPLE_CHECK(castle::bit::popcount(sample) == 5U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_ones(sample) == 5U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_zeros(sample) == 27U);
    CASTLE_SAMPLE_CHECK(castle::bit::count_leading_zeros(sample) == 8U);
    return 0;
}
