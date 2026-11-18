#include "sample_support.hpp"

#include "castle/bit/bit_core.hpp"

#include <stdint.h>

int main()
{
    uint8_t value = 0U;

    CASTLE_SAMPLE_CHECK(!castle::bit::test(value, 0U));
    CASTLE_SAMPLE_CHECK(!castle::bit::test(value, 8U));

    value = castle::bit::set(value, 0U);
    CASTLE_SAMPLE_CHECK(value == static_cast<uint8_t>(0x01U));

    value = castle::bit::set<3U>(value);
    CASTLE_SAMPLE_CHECK(value == static_cast<uint8_t>(0x09U));
    CASTLE_SAMPLE_CHECK(castle::bit::test<3U>(value));

    value = castle::bit::clear(value, 0U);
    CASTLE_SAMPLE_CHECK(value == static_cast<uint8_t>(0x08U));

    value = castle::bit::clear<3U>(value);
    CASTLE_SAMPLE_CHECK(value == static_cast<uint8_t>(0x00U));

    value = castle::bit::toggle(value, 1U);
    CASTLE_SAMPLE_CHECK(value == static_cast<uint8_t>(0x02U));

    value = castle::bit::toggle<1U>(value);
    CASTLE_SAMPLE_CHECK(value == static_cast<uint8_t>(0x00U));

    CASTLE_SAMPLE_CHECK(castle::bit::set(value, 8U) == value);
    CASTLE_SAMPLE_CHECK(castle::bit::clear(value, 8U) == value);
    CASTLE_SAMPLE_CHECK(castle::bit::toggle(value, 8U) == value);

    return 0;
}
