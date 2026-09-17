#include "sample_support.hpp"

#include "castle/utility/pair.hpp"

#include <stdint.h>

int main()
{
    castle::pair<uint32_t, uint16_t> direct(7U, 25U);
    CASTLE_SAMPLE_CHECK(direct.first == 7U);
    CASTLE_SAMPLE_CHECK(direct.second == 25U);

    auto item = castle::make_pair(1U, 2U);
    CASTLE_SAMPLE_CHECK(castle::get<0>(item) == 1U);
    CASTLE_SAMPLE_CHECK(castle::get<1>(item) == 2U);
    CASTLE_SAMPLE_CHECK(castle::get<0>(castle::move(item)) == 1U);

    castle::pair<uint32_t, uint16_t> other(3U, 4U);
    direct.swap(other);
    CASTLE_SAMPLE_CHECK(direct.first == 3U && direct.second == 4U);
    castle::swap(direct, other);
    CASTLE_SAMPLE_CHECK(direct.first == 7U && direct.second == 25U);

    CASTLE_SAMPLE_CHECK((direct == castle::pair<uint32_t, uint16_t>(7U, 25U)));
    CASTLE_SAMPLE_CHECK(direct != other);
    CASTLE_SAMPLE_CHECK(other < direct);
    CASTLE_SAMPLE_CHECK(direct > other);
    CASTLE_SAMPLE_CHECK(other <= direct);
    CASTLE_SAMPLE_CHECK(direct >= other);
    return 0;
}
