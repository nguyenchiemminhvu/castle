#include "sample_support.h"

#include "castle/container/bitset.h"
#include "castle/utility/bitset.h"

// Scenario: exercising both fixed bitset implementations for register maps and protocol masks.
#include <stdint.h>

int main()
{
    // Container bitset: status flags plus string export for a diagnostics frame.
    castle::container::bitset<10U> register_bits(0x205U);
    CASTLE_SAMPLE_CHECK(register_bits.test(0U));
    CASTLE_SAMPLE_CHECK(register_bits.count() == 3U);
    char text[11U] = {};
    CASTLE_SAMPLE_CHECK(register_bits.to_string(text, sizeof(text)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(register_bits.to_uint32() == 0x205U);

    // Utility bitset: configurable storage word width and std::bitset-like API.
    castle::bitset<16U, uint16_t> mask;
    mask.set(3U).set(7U);
    mask[1U] = true;
    mask.flip(7U);
    CASTLE_SAMPLE_CHECK(mask.test(3U));
    CASTLE_SAMPLE_CHECK(mask.test(1U));
    CASTLE_SAMPLE_CHECK(!mask.test(7U));
    CASTLE_SAMPLE_CHECK(mask.count() == 2U);
    CASTLE_SAMPLE_CHECK(mask.to_ullong() == 0x000AU);
    return 0;
}
