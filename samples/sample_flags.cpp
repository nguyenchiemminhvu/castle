/**
 * Castle 2.0 sample: <castle/bit/flags.h>
 *
 * Scenario: represent a four-bit software feature mask while constraining all
 * writes to the declared mask width.
 */
#include "sample_support.h"
#include "castle/bit/flags.h"

#include <stdint.h>

int main()
{
    using mode_flags = castle::bit::flags<uint8_t, 0x0FU>;

    mode_flags flags(0x03U);
    CASTLE_SAMPLE_CHECK(flags.test<0x01U>());
    CASTLE_SAMPLE_CHECK(flags.test<0x02U>());
    CASTLE_SAMPLE_CHECK(!flags.test<0x08U>());

    mode_flags all_set = mode_flags::ALL_SET;
    CASTLE_SAMPLE_CHECK(all_set.test<0x0FU>());
    return 0;
}
