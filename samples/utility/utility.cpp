#include "sample_support.hpp"

#include "castle/utility/utility.hpp"

#include <stdint.h>

int main()
{
    castle::optional<uint8_t> value(3U);
    castle::bitset<4U> flags(0x3U);
    auto pair = castle::make_pair(1U, 2U);

    CASTLE_SAMPLE_CHECK(value.value() == 3U);
    CASTLE_SAMPLE_CHECK(flags.count() == 2U);
    CASTLE_SAMPLE_CHECK(castle::get<1>(pair) == 2U);
    return 0;
}
