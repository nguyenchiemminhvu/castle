#include "sample_support.h"

#include "castle/bit/bit_mask.h"

// Scenario: building compile-time and runtime masks for GPIO and peripheral register fields.
#include <stdint.h>

int main()
{
    static_assert(castle::bit::all_bits_mask<uint8_t>::value == 0xFFU, "all bits");
    static_assert(castle::bit::single_bit_mask_const<3U, uint8_t>::value == 0x08U, "bit mask");
    const uint16_t field = castle::bit::low_bits_mask<uint16_t>(5U);
    CASTLE_SAMPLE_CHECK(field == 0x001FU);
    CASTLE_SAMPLE_CHECK(castle::bit::single_bit_mask<uint32_t>(12U) == 0x1000U);
    return 0;
}
