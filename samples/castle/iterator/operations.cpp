#include "sample_support.hpp"

#include "castle/iterator/operations.hpp"

int main()
{
    uint8_t data[5U] = {1U, 2U, 3U, 4U, 5U};
    uint8_t* it = data;

    castle::advance(it, 3);
    CASTLE_SAMPLE_CHECK(*it == 4U);

    castle::advance(it, -2);
    CASTLE_SAMPLE_CHECK(*it == 2U);
    CASTLE_SAMPLE_CHECK(castle::distance(it, data + 5U) == 4);

    auto third = castle::next(data, 2);
    auto fourth = castle::prev(data + 5U, 2);
    CASTLE_SAMPLE_CHECK(*third == 3U);
    CASTLE_SAMPLE_CHECK(*fourth == 4U);

    return 0;
}
