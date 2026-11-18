#include "sample_support.hpp"

#include "castle/utility/bit_cast.hpp"

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
    const WireWord round_trip = castle::bit_cast<WireWord>(raw);
    CASTLE_SAMPLE_CHECK(raw == 0x12345678U);
    CASTLE_SAMPLE_CHECK(round_trip.value == word.value);
    return 0;
}
