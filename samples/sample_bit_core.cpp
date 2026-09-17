#include "sample_support.h"

#include "castle/bit/bit_core.h"

// Scenario: manipulating MCU control-register bits with bounds-safe bit primitives.
#include <stdint.h>

int main()
{
    uint32_t control = 0U;
    control = castle::bit::set(control, 5U);
    CASTLE_SAMPLE_CHECK(castle::bit::test(control, 5U));
    control = castle::bit::clear(control, 5U);
    CASTLE_SAMPLE_CHECK(!castle::bit::test<5U>(control));
    control = castle::bit::toggle(control, 3U);
    CASTLE_SAMPLE_CHECK(castle::bit::test<3U>(control));
    return 0;
}
