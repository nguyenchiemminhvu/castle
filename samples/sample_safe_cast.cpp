/**
 * Castle 2.0 sample: <castle/utility/safe_cast.h>
 *
 * Scenario: make primitive conversions explicit at a Safety protocol
 * boundary. The example uses non-overflowing values so behavior is unambiguous.
 */
#include "sample_support.h"
#include "castle/utility/safe_cast.h"

#include <stdint.h>

int main()
{
    CASTLE_SAMPLE_CHECK(castle::safe_cast::bool_to_uint8(true) == 1U);
    CASTLE_SAMPLE_CHECK(castle::safe_cast::int8_to_uint8(42) == 42U);
    CASTLE_SAMPLE_CHECK(castle::safe_cast::int16_to_int32(12) == 12);
    return 0;
}
