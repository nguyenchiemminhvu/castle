/**
 * Castle 2.0 sample: <castle/utility/bit_cast.h>
 *
 * Scenario: decoding a fixed-width wire/register representation without C-style
 * aliasing or union type-punning tricks.
 */
#include "sample_support.h"
#include "castle/utility/bit_cast.h"

#include <stdint.h>

struct WireWord
{
    uint32_t value;
};

int main()
{
    const WireWord word{0x12345678U};
    static_assert(sizeof(WireWord) == sizeof(uint32_t), "bit_cast size requirement");

    const uint32_t raw = castle::bit_cast<uint32_t>(word);
    CASTLE_SAMPLE_CHECK(raw == 0x12345678U);
    return 0;
}
